import grpc
import uuid
import time
import csv
import wordcount_pb2
import wordcount_pb2_grpc

REPLICAS = ["localhost:50051", "localhost:50052"]  # 2 replicas
MAX_RETRIES = 3
DURATION = 60  # تشغيل 60 ثانية

end_time = time.time() + DURATION

with open("test_metrics.csv", "w", newline="") as f:
    w = csv.writer(f)
    w.writerow(["ts", "replica", "success", "latency_ms", "input_text"])

    i = 0
    while time.time() < end_time:
        req_id = str(uuid.uuid4())
        input_text = "hello world hello"
        success = False
        start = time.time()

        for attempt in range(MAX_RETRIES):
            addr = REPLICAS[attempt % len(REPLICAS)]
            try:
                ch = grpc.insecure_channel(addr)
                stub = wordcount_pb2_grpc.WordCountServiceStub(ch)
                r = stub.CountWords(
                    wordcount_pb2.WordCountRequest(text=input_text, request_id=req_id),
                    timeout=2.0
                )
                latency_ms = r.latency_ms if hasattr(r, "latency_ms") else (time.time()-start)*1000
                w.writerow([time.time(), addr, True, latency_ms, input_text])
                print(f"{i}: OK {addr} '{r.output}' {latency_ms:.1f}ms")
                success = True
                break
            except grpc.RpcError as e:
                print(f"{i}: FAIL {addr} {e.code()}")
                continue

        if not success:
            w.writerow([time.time(), None, False, (time.time()-start)*1000, input_text])
            print(f"{i}: ALL REPLICAS FAILED")

        i += 1
        time.sleep(0.5)

print("Test complete. Check test_metrics.csv")
