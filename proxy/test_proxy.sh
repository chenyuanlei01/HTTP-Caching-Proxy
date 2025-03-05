#!/bin/bash
# filepath: /home/ar791/ece568/erss-hwk2-cl676-ar791/test_proxy.sh

set -e

PROXY="http://localhost:12345"
TEST_URL="http://example.com"
LOG_FILE="./logs/proxy.log"

echo "Starting HTTP proxy integration tests..."

# Ensure the proxy is running
if ! curl --head --proxy $PROXY $TEST_URL -m 5 > /dev/null 2>&1; then
  echo "Error: Proxy doesn't appear to be running. Start it with 'docker-compose up'"
  exit 1
fi

# Test 1: Basic GET request
echo "Test 1: Basic GET request"
curl -s --proxy $PROXY $TEST_URL > /dev/null
echo "✅ Basic GET successful"

# Test 2: Cache hit (same request)
echo "Test 2: Cache hit test"
curl -s --proxy $PROXY $TEST_URL > /dev/null
if grep -q "in cache, valid" $LOG_FILE; then
  echo "✅ Cache hit successful"
else
  echo "❌ Cache hit failed"
fi

# Test 3: POST request
echo "Test 3: POST request"
curl -s -X POST --proxy $PROXY -d "test data" $TEST_URL > /dev/null
echo "✅ POST successful"

# Test 4: HTTPS CONNECT request
echo "Test 4: HTTPS CONNECT request"
curl -s --proxy $PROXY https://example.com > /dev/null
if grep -q "Connection Established" $LOG_FILE; then
  echo "✅ CONNECT successful"
else
  echo "❌ CONNECT failed"
fi

# Test 5: Multiple concurrent requests
echo "Test 5: Multiple concurrent requests"
for i in {1..5}; do
  timeout 10 curl -s --proxy $PROXY "http://example.com/$i" > /dev/null &
done
wait
echo "✅ Multiple requests completed"

# Test 6: Check cache handling (non-cacheable)
echo "Test 6: Handling non-cacheable resources"
curl -s --proxy $PROXY "http://httpbin.org/response-headers?Cache-Control=no-store" > /dev/null

if grep -q "not cacheable" $LOG_FILE; then
  echo "✅ Non-cacheable handling successful"
else
  echo "❌ Non-cacheable handling check failed"
fi

# Test 7: Error handling with non-existent domain
echo "Test 7: Error handling"
curl -s --proxy $PROXY "http://non-existent-domain-12345.com" > /dev/null 2>&1 || true
echo "✅ Error handling completed"

echo "All tests completed!"