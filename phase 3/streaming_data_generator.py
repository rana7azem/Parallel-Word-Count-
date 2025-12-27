import os
import time
import argparse
from datetime import datetime
import uuid

SAMPLE_TEXTS = [
    "hello world hello",
    "the quick brown fox jumps over the lazy dog",
    "parallel computing is fascinating",
    "word count word count word count",
    "distributed systems require careful design",
    "spark streaming processes data in micro batches",
    "grpc provides efficient communication between services",
    "microservices architecture enables scalability",
    "real time processing requires low latency",
    "big data analytics transforms business decisions"
]


def generate_streaming_file(output_dir, file_index):
    os.makedirs(output_dir, exist_ok=True)
    
    filename = f"stream_{file_index:06d}_{int(time.time())}.txt"
    filepath = os.path.join(output_dir, filename)
    
    import random
    num_lines = random.randint(5, 10)
    
    with open(filepath, 'w') as f:
        for i in range(num_lines):
            text = random.choice(SAMPLE_TEXTS)
            timestamp = datetime.now().isoformat()
            f.write(f"{timestamp},{text}\n")
    
    print(f"[{datetime.now().isoformat()}] Generated: {filename} ({num_lines} lines)")
    return filepath


def main():
    parser = argparse.ArgumentParser(description='Generate streaming data files for Spark')
    parser.add_argument('--output-dir', type=str, default='data/streaming_input',
                       help='Output directory for streaming files (default: data/streaming_input)')
    parser.add_argument('--interval', type=float, default=3.0,
                       help='Interval between file generation in seconds (default: 3.0)')
    parser.add_argument('--duration', type=int, default=60,
                       help='Duration to generate files in seconds (default: 60)')
    parser.add_argument('--clean', action='store_true',
                       help='Clean output directory before starting')
    
    args = parser.parse_args()
    
    if args.clean and os.path.exists(args.output_dir):
        import shutil
        shutil.rmtree(args.output_dir)
        print(f"Cleaned directory: {args.output_dir}")
    
    print(f"[{datetime.now().isoformat()}] Starting data generator...")
    print(f"Output directory: {args.output_dir}")
    print(f"Interval: {args.interval}s")
    print(f"Duration: {args.duration}s")
    print("-" * 60)
    
    start_time = time.time()
    end_time = start_time + args.duration
    file_index = 0
    
    try:
        while time.time() < end_time:
            generate_streaming_file(args.output_dir, file_index)
            file_index += 1
            time.sleep(args.interval)
    except KeyboardInterrupt:
        print(f"\n[{datetime.now().isoformat()}] Stopped by user")
    
    print(f"\n[{datetime.now().isoformat()}] Generated {file_index} files")
    print(f"Files are in: {args.output_dir}")


if __name__ == "__main__":
    main()
