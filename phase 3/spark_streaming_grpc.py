import os
import sys
from pyspark.sql import SparkSession
from pyspark.sql.functions import col, udf, explode, split, count, sum as spark_sum
from pyspark.sql.types import StringType, StructType, StructField, TimestampType
import grpc
import wordcount_pb2
import wordcount_pb2_grpc
from datetime import datetime
import time


GRPC_SERVER = os.getenv("GRPC_SERVER", "localhost:50051")
INPUT_DIR = os.getenv("INPUT_DIR", "data/streaming_input")
CHECKPOINT_DIR = os.getenv("CHECKPOINT_DIR", "checkpoints/spark_streaming")
OUTPUT_MODE = os.getenv("OUTPUT_MODE", "update")
TRIGGER_INTERVAL = os.getenv("TRIGGER_INTERVAL", "5 seconds")


def make_grpc_call(text):
    try:
        channel = grpc.insecure_channel(GRPC_SERVER)
        stub = wordcount_pb2_grpc.WordCountServiceStub(channel)
        
        request = wordcount_pb2.WordCountRequest(
            text=text,
            request_id=f"spark_{int(time.time() * 1000)}"
        )
        
        response = stub.CountWords(request, timeout=5.0)
        channel.close()
        
        return response.output
        
    except Exception as e:
        print(f"gRPC call failed: {str(e)}", file=sys.stderr)
        return f"ERROR: {str(e)}"


grpc_call_udf = udf(make_grpc_call, StringType())


def create_spark_session():
    spark = SparkSession.builder \
        .appName("WordCountStreamingGRPC") \
        .config("spark.sql.streaming.checkpointLocation", CHECKPOINT_DIR) \
        .config("spark.sql.streaming.schemaInference", "true") \
        .getOrCreate()
    
    spark.sparkContext.setLogLevel("WARN")
    return spark


def process_microbatch_with_grpc(spark):
    schema = StructType([
        StructField("timestamp", TimestampType(), True),
        StructField("text", StringType(), True)
    ])
    
    streaming_df = spark.readStream \
        .format("text") \
        .option("path", INPUT_DIR) \
        .option("maxFilesPerTrigger", 1) \
        .load()
    
    from pyspark.sql.functions import regexp_extract
    parsed_df = streaming_df.select(
        regexp_extract(col("value"), r"^[^,]+,(.+)$", 1).alias("text")
    ).filter(col("text") != "")
    
    from pyspark.sql.functions import current_timestamp
    parsed_df = parsed_df.withColumn("processing_time", current_timestamp())
    
    print("Making gRPC calls for micro-batch...")
    result_df = parsed_df.withColumn("grpc_response", grpc_call_udf(col("text")))
    
    result_df = result_df.select(
        col("text"),
        col("processing_time"),
        col("grpc_response"),
        explode(split(col("grpc_response"), " ")).alias("word_count_pair")
    ).filter(col("word_count_pair") != "")
    
    result_df = result_df.select(
        col("text"),
        col("processing_time"),
        split(col("word_count_pair"), ":").getItem(0).alias("word"),
        split(col("word_count_pair"), ":").getItem(1).cast("int").alias("count")
    )
    
    aggregated_df = result_df.groupBy("word") \
        .agg(
            spark_sum("count").alias("total_count"),
            count("*").alias("occurrences")
        ) \
        .orderBy(col("total_count").desc())
    
    return aggregated_df


def write_streaming_output(query):
    query = query.writeStream \
        .outputMode(OUTPUT_MODE) \
        .format("console") \
        .option("truncate", "false") \
        .option("numRows", 50) \
        .trigger(processingTime=TRIGGER_INTERVAL) \
        .start()
    
    return query


def main():
    print("=" * 60)
    print("Spark Structured Streaming with gRPC Integration")
    print("=" * 60)
    print(f"gRPC Server: {GRPC_SERVER}")
    print(f"Input Directory: {INPUT_DIR}")
    print(f"Checkpoint Directory: {CHECKPOINT_DIR}")
    print(f"Trigger Interval: {TRIGGER_INTERVAL}")
    print("=" * 60)
    
    spark = create_spark_session()
    
    try:
        result_df = process_microbatch_with_grpc(spark)
        
        query = write_streaming_output(result_df)
        
        print("\nStreaming query started. Waiting for data...")
        print("Press Ctrl+C to stop.\n")
        
        query.awaitTermination()
        
    except KeyboardInterrupt:
        print("\n\nStopping streaming query...")
        query.stop()
        print("Streaming stopped.")
    except Exception as e:
        print(f"\nError: {str(e)}", file=sys.stderr)
        raise
    finally:
        spark.stop()


if __name__ == "__main__":
    main()
