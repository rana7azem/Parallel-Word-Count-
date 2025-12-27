import grpc
import uuid
import time
import wordcount_pb2
import wordcount_pb2_grpc

REPLICAS = [
    "localhost:50051",
    "localhost:50052"
]
MAX_RETRIES = 3


for i in range(5):
    request_id = str(uuid.uuid4())
    text = "hello world hello"
    success = False

    for attempt in range(MAX_RETRIES):
        addr = REPLICAS[attempt % len(REPLICAS)]

        try:
            channel = grpc.insecure_channel(addr)
            stub = wordcount_pb2_grpc.WordCountServiceStub(channel)

            response = stub.CountWords(
                wordcount_pb2.WordCountRequest(
                    text=text,
                    request_id=request_id
                ),
                timeout=1.0
            )

            print(f"{i}: OK from {addr} → {response.output}")
            success = True
            break

        except grpc.RpcError as e:
            print(f"{i}: FAIL from {addr} ({e.code()})")

    if not success:
        print(f"{i}: ALL ATTEMPTS FAILED")

    time.sleep(0.5)
