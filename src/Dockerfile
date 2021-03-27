# Get the GCC preinstalled image from Docker Hub
FROM gcc:latest

# Specify the working directory
WORKDIR /src

# Copy the current folder which contains C++ source code to the Docker image under /src
COPY . .

# Use GCC to compile the server.cpp source file
RUN g++ -pthread -std=c++11 easylogging++.cc -o server server.cpp

# Expose ports
EXPOSE 3478
EXPOSE 5349

# Run the program and set the -r flag
ENTRYPOINT ["./server", "-r"]

# Specify default argument to be used with the -r flag. Can be overwritten
CMD ["udp"]