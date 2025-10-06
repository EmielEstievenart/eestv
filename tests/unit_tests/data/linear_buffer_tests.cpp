#include <gtest/gtest.h>
#include "eestv/data/linear_buffer.hpp"
#include <cstring>
#include <string>

class LinearBufferTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Create a buffer with 100 bytes capacity for most tests
        buffer = std::make_unique<LinearBuffer>(100);
    }

    void TearDown() override { buffer.reset(); }

    // Helper function to push data (mimics old push behavior)
    bool push_data(const void* data, std::size_t size)
    {
        if (data == nullptr || size == 0)
        {
            return false;
        }

        std::size_t writable;
        std::uint8_t* write_head = buffer->get_write_head(writable);
        if (write_head == nullptr || writable < size)
        {
            return false;
        }

        std::memcpy(write_head, data, size);
        return buffer->commit(size);
    }

    std::unique_ptr<LinearBuffer> buffer;
};

// Basic construction and initial state tests
TEST_F(LinearBufferTest, InitialState)
{
    std::size_t read_size;
    std::size_t write_size;

    EXPECT_EQ(buffer->get_read_head(read_size), nullptr);
    EXPECT_EQ(read_size, 0);
    EXPECT_NE(buffer->get_write_head(write_size), nullptr);
    EXPECT_EQ(write_size, 100);
}

TEST_F(LinearBufferTest, ConstructorWithZeroSize)
{
    LinearBuffer zero_buffer(0);
    std::size_t read_size;
    std::size_t write_size;

    EXPECT_EQ(zero_buffer.get_read_head(read_size), nullptr);
    EXPECT_EQ(read_size, 0);
    EXPECT_EQ(zero_buffer.get_write_head(write_size), nullptr);
    EXPECT_EQ(write_size, 0);
}

// Push operation tests (now using get_write_head + commit)
TEST_F(LinearBufferTest, PushValidData)
{
    const char* test_data = "Hello";
    std::size_t data_size = 5;

    std::size_t writable;
    std::uint8_t* write_head = buffer->get_write_head(writable);
    ASSERT_NE(write_head, nullptr);
    ASSERT_GE(writable, data_size);
    std::memcpy(write_head, test_data, data_size);
    EXPECT_TRUE(buffer->commit(data_size));

    std::size_t read_size;
    std::size_t write_size;
    EXPECT_NE(buffer->get_read_head(read_size), nullptr);
    EXPECT_EQ(read_size, data_size);
    EXPECT_NE(buffer->get_write_head(write_size), nullptr);
    EXPECT_EQ(write_size, 100 - data_size);
}

TEST_F(LinearBufferTest, PushNullPointer)
{
    EXPECT_FALSE(push_data(nullptr, 5));

    std::size_t read_size;
    EXPECT_EQ(buffer->get_read_head(read_size), nullptr);
    EXPECT_EQ(read_size, 0);
}

TEST_F(LinearBufferTest, PushZeroSize)
{
    const char* test_data = "Hello";
    EXPECT_FALSE(push_data(test_data, 0));

    std::size_t read_size;
    EXPECT_EQ(buffer->get_read_head(read_size), nullptr);
    EXPECT_EQ(read_size, 0);
}

TEST_F(LinearBufferTest, PushExceedsCapacity)
{
    // Try to push more data than buffer capacity
    std::string large_data(150, 'X');
    EXPECT_FALSE(push_data(large_data.data(), large_data.size()));

    std::size_t read_size;
    EXPECT_EQ(buffer->get_read_head(read_size), nullptr);
    EXPECT_EQ(read_size, 0);
}

TEST_F(LinearBufferTest, PushUntilFull)
{
    std::string data(100, 'A');
    EXPECT_TRUE(push_data(data.data(), data.size()));

    std::size_t write_size;
    EXPECT_EQ(buffer->get_write_head(write_size), nullptr);
    EXPECT_EQ(write_size, 0);

    // Try to push one more byte
    char extra = 'B';
    EXPECT_FALSE(push_data(&extra, 1));
}

TEST_F(LinearBufferTest, PushMultipleChunks)
{
    const char* chunk1 = "Hello";
    const char* chunk2 = " ";
    const char* chunk3 = "World";

    EXPECT_TRUE(push_data(chunk1, 5));
    EXPECT_TRUE(push_data(chunk2, 1));
    EXPECT_TRUE(push_data(chunk3, 5));

    std::size_t read_size;
    std::size_t write_size;
    EXPECT_NE(buffer->get_read_head(read_size), nullptr);
    EXPECT_EQ(read_size, 11);
    EXPECT_NE(buffer->get_write_head(write_size), nullptr);
    EXPECT_EQ(write_size, 89);
}

// Read head operation tests (replaces peek tests)
TEST_F(LinearBufferTest, GetReadHeadEmptyBuffer)
{
    std::size_t size_out;
    const std::uint8_t* data = buffer->get_read_head(size_out);

    EXPECT_EQ(data, nullptr);
    EXPECT_EQ(size_out, 0);
}

TEST_F(LinearBufferTest, GetReadHeadWithData)
{
    const char* test_data = "Hello World";
    std::size_t data_size = 11;

    push_data(test_data, data_size);

    std::size_t size_out;
    const std::uint8_t* data = buffer->get_read_head(size_out);

    EXPECT_NE(data, nullptr);
    EXPECT_EQ(size_out, data_size);

    // Verify the data content
    const char* char_data = reinterpret_cast<const char*>(data);
    EXPECT_EQ(std::strncmp(char_data, test_data, data_size), 0);
}

TEST_F(LinearBufferTest, GetReadHeadContiguousData)
{
    // Push multiple chunks and verify they're accessible as contiguous memory
    push_data("Hello", 5);
    push_data(" ", 1);
    push_data("World", 5);

    std::size_t size_out;
    const std::uint8_t* data = buffer->get_read_head(size_out);

    EXPECT_NE(data, nullptr);
    EXPECT_EQ(size_out, 11);

    // Verify all data is contiguous
    const char* char_data = reinterpret_cast<const char*>(data);
    std::string result(char_data, size_out);
    EXPECT_EQ(result, "Hello World");
}

// Consume operation tests
TEST_F(LinearBufferTest, ConsumeFromEmptyBuffer)
{
    EXPECT_FALSE(buffer->consume(1));

    std::size_t read_size;
    EXPECT_EQ(buffer->get_read_head(read_size), nullptr);
    EXPECT_EQ(read_size, 0);
}

TEST_F(LinearBufferTest, ConsumeMoreThanAvailable)
{
    push_data("Hello", 5);
    EXPECT_FALSE(buffer->consume(10));

    std::size_t read_size;
    EXPECT_NE(buffer->get_read_head(read_size), nullptr);
    EXPECT_EQ(read_size, 5); // Data should remain unchanged
}

TEST_F(LinearBufferTest, ConsumePartialData)
{
    push_data("Hello World", 11);

    EXPECT_TRUE(buffer->consume(6)); // Consume "Hello "

    std::size_t size_out;
    const std::uint8_t* data = buffer->get_read_head(size_out);
    const char* char_data    = reinterpret_cast<const char*>(data);

    EXPECT_EQ(size_out, 5);
    EXPECT_EQ(std::strncmp(char_data, "World", 5), 0);
}

TEST_F(LinearBufferTest, ConsumeAllData)
{
    push_data("Hello", 5);
    EXPECT_TRUE(buffer->consume(5));

    std::size_t read_size;
    std::size_t write_size;
    EXPECT_EQ(buffer->get_read_head(read_size), nullptr);
    EXPECT_EQ(read_size, 0);
    EXPECT_NE(buffer->get_write_head(write_size), nullptr);
    EXPECT_EQ(write_size, 100); // Should have full capacity again
}

// Reset behavior tests
TEST_F(LinearBufferTest, ResetOnCompleteConsumption)
{
    // Fill buffer partially
    push_data("Test", 4);
    std::size_t read_size;
    EXPECT_NE(buffer->get_read_head(read_size), nullptr);
    EXPECT_EQ(read_size, 4);

    // Consume all data
    EXPECT_TRUE(buffer->consume(4));
    EXPECT_EQ(buffer->get_read_head(read_size), nullptr);
    EXPECT_EQ(read_size, 0);

    // After reset, we should be able to use full capacity again
    std::string large_data(100, 'X');
    EXPECT_TRUE(push_data(large_data.data(), large_data.size()));

    std::size_t write_size;
    EXPECT_EQ(buffer->get_write_head(write_size), nullptr);
    EXPECT_EQ(write_size, 0);
}

TEST_F(LinearBufferTest, NoResetOnPartialConsumption)
{
    push_data("Hello World", 11);
    buffer->consume(6); // Consume "Hello "

    // Should still have "World" available
    std::size_t size_out;
    const std::uint8_t* data = buffer->get_read_head(size_out);
    EXPECT_EQ(size_out, 5);

    const char* char_data = reinterpret_cast<const char*>(data);
    EXPECT_EQ(std::strncmp(char_data, "World", 5), 0);
}

// Edge case tests
TEST_F(LinearBufferTest, InsufficientContiguousSpace)
{
    // Fill buffer to near capacity
    std::string data1(90, 'A');
    push_data(data1.data(), data1.size());

    // Consume some from the beginning
    buffer->consume(50);

    // Now we have 40 bytes of data and 60 bytes free, but only 10 bytes contiguous at the end
    // Try to push 20 bytes (should fail due to lack of contiguous space)
    std::string data2(20, 'B');
    EXPECT_FALSE(push_data(data2.data(), data2.size()));

    // Should still have the original remaining data
    std::size_t read_size;
    EXPECT_NE(buffer->get_read_head(read_size), nullptr);
    EXPECT_EQ(read_size, 40);
}

TEST_F(LinearBufferTest, PushAfterReset)
{
    // Use buffer, consume all, then reuse
    push_data("First", 5);
    buffer->consume(5);

    std::size_t read_size;
    EXPECT_EQ(buffer->get_read_head(read_size), nullptr);
    EXPECT_EQ(read_size, 0);

    // After reset, should be able to push new data from the beginning
    push_data("Second", 6);

    std::size_t size_out;
    const std::uint8_t* data = buffer->get_read_head(size_out);
    const char* char_data    = reinterpret_cast<const char*>(data);

    EXPECT_EQ(size_out, 6);
    EXPECT_EQ(std::strncmp(char_data, "Second", 6), 0);
}

// Stress test with multiple operations
TEST_F(LinearBufferTest, MultipleOperationSequence)
{
    // Push, read, consume in various combinations
    push_data("ABC", 3);
    push_data("DEF", 3);

    std::size_t size_out;
    const std::uint8_t* data = buffer->get_read_head(size_out);
    EXPECT_EQ(size_out, 6);

    buffer->consume(2); // Consume "AB"
    data = buffer->get_read_head(size_out);
    EXPECT_EQ(size_out, 4);

    const char* char_data = reinterpret_cast<const char*>(data);
    EXPECT_EQ(std::strncmp(char_data, "CDEF", 4), 0);

    push_data("GHI", 3);
    data = buffer->get_read_head(size_out);
    EXPECT_EQ(size_out, 7);

    char_data = reinterpret_cast<const char*>(data);
    EXPECT_EQ(std::strncmp(char_data, "CDEFGHI", 7), 0);
}

// New interface tests for TCP connection compatibility
TEST_F(LinearBufferTest, GetWriteHeadInitial)
{
    std::size_t write_size;
    std::uint8_t* write_head = buffer->get_write_head(write_size);
    EXPECT_NE(write_head, nullptr);
    EXPECT_EQ(write_size, 100);
}

TEST_F(LinearBufferTest, GetWriteHeadAfterPush)
{
    push_data("Hello", 5);

    std::size_t write_size;
    std::uint8_t* write_head = buffer->get_write_head(write_size);
    EXPECT_NE(write_head, nullptr);
    EXPECT_EQ(write_size, 95);

    // Write directly to the write head
    const char* more_data = " World";
    std::memcpy(write_head, more_data, 6);
    EXPECT_TRUE(buffer->commit(6));

    // Verify the combined data
    std::size_t size_out;
    const std::uint8_t* data = buffer->get_read_head(size_out);
    EXPECT_EQ(size_out, 11);
    EXPECT_EQ(std::strncmp(reinterpret_cast<const char*>(data), "Hello World", 11), 0);
}

TEST_F(LinearBufferTest, WritableBytesConsidersContiguousSpace)
{
    // Fill buffer to 90 bytes
    std::string data1(90, 'A');
    push_data(data1.data(), 90);

    // We have 10 bytes free, and they're contiguous at the end
    std::size_t write_size;
    EXPECT_NE(buffer->get_write_head(write_size), nullptr);
    EXPECT_EQ(write_size, 10);
}

TEST_F(LinearBufferTest, SendDataInterface)
{
    const char* test_data = "Test Message";
    push_data(test_data, 12);

    // get_read_head() provides the data for sending
    std::size_t read_size;
    std::size_t send_size;
    const std::uint8_t* read_ptr = buffer->get_read_head(read_size);
    const std::uint8_t* send_ptr = buffer->get_read_head(send_size);

    EXPECT_EQ(send_ptr, read_ptr);
    EXPECT_EQ(send_size, read_size);
    EXPECT_EQ(read_size, 12);
    EXPECT_EQ(send_size, 12);

    const char* send_char_ptr = reinterpret_cast<const char*>(send_ptr);
    EXPECT_EQ(std::strncmp(send_char_ptr, test_data, 12), 0);
}

TEST_F(LinearBufferTest, DirectWriteViaWriteHead)
{
    // Simulate what Boost.Asio would do: get write head, write data, commit
    std::size_t writable;
    std::uint8_t* write_ptr = buffer->get_write_head(writable);

    EXPECT_EQ(writable, 100);

    // Write some data
    const char* message = "Direct write test";
    std::size_t msg_len = 17;
    std::memcpy(write_ptr, message, msg_len);

    // Commit the written data
    EXPECT_TRUE(buffer->commit(msg_len));

    // Verify
    std::size_t readable;
    const std::uint8_t* read_ptr = buffer->get_read_head(readable);
    EXPECT_EQ(readable, msg_len);
    const char* read_char_ptr = reinterpret_cast<const char*>(read_ptr);
    EXPECT_EQ(std::strncmp(read_char_ptr, message, msg_len), 0);
}

TEST_F(LinearBufferTest, ConsumeAndReadHeadAdvance)
{
    push_data("ABCDEFGHIJ", 10);

    std::size_t initial_size;
    const std::uint8_t* initial_read = buffer->get_read_head(initial_size);
    EXPECT_EQ(initial_size, 10);

    // Consume 3 bytes
    EXPECT_TRUE(buffer->consume(3));

    // Read head should have advanced
    std::size_t new_size;
    const std::uint8_t* new_read = buffer->get_read_head(new_size);
    EXPECT_EQ(new_read, initial_read + 3);
    EXPECT_EQ(new_size, 7);

    // Verify remaining data
    const char* remaining = reinterpret_cast<const char*>(new_read);
    EXPECT_EQ(std::strncmp(remaining, "DEFGHIJ", 7), 0);
}

TEST_F(LinearBufferTest, FullWriteReadCycle)
{
    // Simulate a full TCP send/receive cycle

    // 1. Receive data (write to buffer)
    const char* received = "Incoming data";
    std::size_t writable;
    std::uint8_t* write_head = buffer->get_write_head(writable);
    std::memcpy(write_head, received, 13);
    EXPECT_TRUE(buffer->commit(13));

    // 2. Read data for processing
    std::size_t send_size;
    const std::uint8_t* read_head = buffer->get_read_head(send_size);
    EXPECT_EQ(send_size, 13);
    EXPECT_EQ(std::strncmp(reinterpret_cast<const char*>(read_head), received, 13), 0);

    // 3. Consume processed data
    EXPECT_TRUE(buffer->consume(13));

    std::size_t read_size;
    EXPECT_EQ(buffer->get_read_head(read_size), nullptr);
    EXPECT_EQ(read_size, 0);

    // 4. Buffer should reset, allowing full capacity again
    std::size_t write_size;
    EXPECT_NE(buffer->get_write_head(write_size), nullptr);
    EXPECT_EQ(write_size, 100);
}
