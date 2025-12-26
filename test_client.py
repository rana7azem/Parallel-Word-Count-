



import grpc
import uuid
import time
import csv
import wordcount_pb2
import wordcount_pb2_grpc
import sys


REPLICAS = ["localhost:50051"]  #

MAX_RETRIES = 3

with open("test_metrics.csv", "w", newline="") as f:
    w = csv.writer(f)
    w.writerow(["ts", "replica", "success", "latency_ms", "input_text"])
    
    for i in range(10):  #
        req_id = str(uuid.uuid4())
        input_text = "hello world hello"  # input
        success = False
        start = time.time()
        
      
        for attempt in range(MAX_RETRIES):
            addr = REPLICAS[attempt % len(REPLICAS)]
            try:
                ch = grpc.insecure_channel(addr)
                stub = wordcount_pb2_grpc.WordCountServiceStub(ch)
                r = stub.CountWords(
                    wordcount_pb2.WordCountRequest(
                        text=input_text, 
                        request_id=req_id
                    ),
                    timeout=1.0
                )
                success = True
                w.writerow([time.time(), addr, True, r.latency_ms, input_text])
                print(f"{i}: OK {addr} '{r.output}' {r.latency_ms:.1f}ms")
                break
            except grpc.RpcError as e:
                print(f"{i}: FAIL {addr} {e.code()}")
                continue
        
        if not success:
            w.writerow([time.time(), None, False, (time.time()-start)*1000, input_text])
            print(f"{i}: ALL REPLICAS FAILED")
        
        time.sleep(0.5)  

print("Test complete. Check test_metrics.csv")

