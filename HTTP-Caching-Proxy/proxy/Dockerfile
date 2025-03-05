FROM ubuntu:22.04

# Set noninteractive installation
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    g++ \
    make \
    netcat \
    libboost-all-dev \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy all files from proxy directory
COPY . .

# Create both log directories to ensure compatibility
RUN mkdir -p /var/log/erss && chmod -R 777 /var/log/erss
RUN mkdir -p ./logs && chmod -R 777 ./logs

# Create a symlink to ensure paths work
RUN ln -sf /var/log/erss/proxy.log ./logs/proxy.log

# Build the proxy
RUN make

# Expose the port that the proxy will run on
# Adjust this port number if your proxy uses a different port
EXPOSE 12345

# Command to run the proxy
# Adjust the command based on your actual executable name and arguments
CMD ["./proxy"] 