import time
import grpc
import subprocess
import tempfile
import os
from concurrent import futures
import wordcount_pb2
import wordcount_pb2_grpc

processed = {}  # Idempotency cache

class WordCountService(wordcount_pb2_grpc.WordCountServiceServicer):
    def CountWords(self, request, context):
        start = time.time()
        
        # Idempotency check
        if request.request_id in processed:
            output = processed[request.request_id]
        else:
            try:
                # Write text to temporary file
                with tempfile.NamedTemporaryFile(mode='w', delete=False, suffix='.txt') as tmp_file:
                    tmp_file.write(request.text)
                    tmp_file_path = tmp_file.name
                
                try:
                    # Run MPI word count binary
                    # Using 4 processes by default, can be made configurable
                    result = subprocess.run(
                        ['mpiexec', '-n', '4', './wc_mpi.exe', tmp_file_path],
                        capture_output=True,
                        text=True,
                        timeout=30,
                        cwd=os.path.dirname(os.path.abspath(__file__))
                    )
                    
                    if result.returncode != 0:
                        context.set_code(grpc.StatusCode.INTERNAL)
                        context.set_details(f"MPI execution failed: {result.stderr}")
                        return wordcount_pb2.WordCountResponse(output="", latency_ms=0.0)
                    
                    # Parse output: convert "word -> count" to "word:count"
                    lines = result.stdout.strip().split('\n')
                    word_counts = []
                    
                    for line in lines:
                        # Skip header lines
                        if '=====' in line or not line.strip():
                            continue
                        # Parse "word -> count" format
                        if ' -> ' in line:
                            parts = line.split(' -> ')
                            if len(parts) == 2:
                                word = parts[0].strip()
                                count = parts[1].strip()
                                word_counts.append(f"{word}:{count}")
                    
                    output = " ".join(word_counts)
                    processed[request.request_id] = output
                    
                finally:
                    # Clean up temporary file
                    if os.path.exists(tmp_file_path):
                        os.unlink(tmp_file_path)
                        
            except subprocess.TimeoutExpired:
                context.set_code(grpc.StatusCode.DEADLINE_EXCEEDED)
                context.set_details("MPI execution timed out")
                return wordcount_pb2.WordCountResponse(output="", latency_ms=0.0)
            except Exception as e:
                context.set_code(grpc.StatusCode.INVALID_ARGUMENT)
                context.set_details(f"Error processing request: {str(e)}")
                return wordcount_pb2.WordCountResponse(output="", latency_ms=0.0)
        
        latency = (time.time() - start) * 1000
        print(f"{time.strftime('%H:%M:%S')} req latency: {latency:.1f}ms")
        return wordcount_pb2.WordCountResponse(output=output, latency_ms=latency)
    
    def Health(self, request, context):
        return wordcount_pb2.HealthStatus(alive=True)

def serve(port):
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    wordcount_pb2_grpc.add_WordCountServiceServicer_to_server(WordCountService(), server)
    server.add_insecure_port(f"[::]:{port}")
    server.start()
    print(f"Server listening on port {port}")
    server.wait_for_termination()

if __name__ == '__main__':
    import sys
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 50051
    serve(port)

