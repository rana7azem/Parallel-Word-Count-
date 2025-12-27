import time
import grpc
from concurrent import futures
import wordcount_pb2
import wordcount_pb2_grpc

class WordCountService(wordcount_pb2_grpc.WordCountServiceServicer):

    def CountWords(self, request, context):
        start = time.time()

        # simulate processing
        time.sleep(0.2)

        words = request.text.split()
        counts = {}
        for w in words:
            counts[w] = counts.get(w, 0) + 1

        output = " ".join([f"{k}:{v}" for k, v in counts.items()])
        latency = (time.time() - start) * 1000

        return wordcount_pb2.WordCountResponse(
            output=output,
            latency_ms=latency
        )

    def Health(self, request, context):
        return wordcount_pb2.HealthStatus(alive=True)

def serve(port):
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    wordcount_pb2_grpc.add_WordCountServiceServicer_to_server(
        WordCountService(), server
    )
    server.add_insecure_port(f"[::]:{port}")
    server.start()
    print(f"✅ Server running on port {port}")
    server.wait_for_termination()

if __name__ == "__main__":
    import sys
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 50051
    serve(port)
