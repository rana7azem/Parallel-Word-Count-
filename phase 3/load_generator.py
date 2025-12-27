"""
Input Load Generator for Word Count Service

Simulates continuous incoming requests (streaming) with:
- Adjustable input rate
- Minimum 60 seconds duration
- Logged timestamps for all events
"""

import grpc
import uuid
import time
import argparse
from datetime import datetime
import wordcount_pb2
import wordcount_pb2_grpc

# Sample texts for word counting
SAMPLE_TEXTS = [
    "hello world hello",
    "the quick brown fox jumps over the lazy dog",
    "parallel computing is fascinating",
    "word count word count word count",
    "distributed systems require careful design"
]


def send_request(server_address, request_id, text):
    """Send a single request and log with timestamp."""
    timestamp = datetime.now().isoformat()
    print(f"[{timestamp}] [SENT] Request ID: {request_id}, Text: '{text[:40]}...'")
    
    try:
        channel = grpc.insecure_channel(server_address)
        stub = wordcount_pb2_grpc.WordCountServiceStub(channel)
        
        start_time = time.time()
        response = stub.CountWords(
            wordcount_pb2.WordCountRequest(text=text, request_id=request_id),
            timeout=5.0
        )
        latency_ms = (time.time() - start_time) * 1000
        
        timestamp = datetime.now().isoformat()
        print(f"[{timestamp}] [SUCCESS] Request ID: {request_id}, "
              f"Latency: {latency_ms:.2f}ms, Response: '{response.output[:50]}...'")
        
        channel.close()
        return True
        
    except Exception as e:
        timestamp = datetime.now().isoformat()
        print(f"[{timestamp}] [FAILED] Request ID: {request_id}, Error: {str(e)}")
        return False


def main():
    parser = argparse.ArgumentParser(description='Input Load Generator for Word Count Service')
    parser.add_argument('--server', type=str, default='localhost:50051',
                       help='gRPC server address (default: localhost:50051)')
    parser.add_argument('--rate', type=float, default=10.0,
                       help='Request rate in requests per second (default: 10.0)')
    parser.add_argument('--duration', type=int, default=60,
                       help='Duration in seconds (minimum: 60, default: 60)')
    
    args = parser.parse_args()
    
    # Ensure minimum 60 seconds
    duration = max(args.duration, 60)
    rate = args.rate
    interval = 1.0 / rate  # Time between requests
    
    print(f"[{datetime.now().isoformat()}] [START] Load generator starting...")
    print(f"[{datetime.now().isoformat()}] [CONFIG] Server: {args.server}, "
          f"Rate: {rate} req/s, Duration: {duration}s")
    
    start_time = time.time()
    end_time = start_time + duration
    request_count = 0
    text_index = 0
    
    # Continuous streaming requests
    while time.time() < end_time:
        request_id = str(uuid.uuid4())
        text = SAMPLE_TEXTS[text_index % len(SAMPLE_TEXTS)]
        text_index += 1
        
        # Send request (non-blocking for rate control)
        send_request(args.server, request_id, text)
        request_count += 1
        
        # Wait to maintain rate
        time.sleep(interval)
    
    print(f"[{datetime.now().isoformat()}] [STOP] Load generator finished")
    print(f"[{datetime.now().isoformat()}] [STATS] Total requests sent: {request_count}")


if __name__ == "__main__":
    main()
