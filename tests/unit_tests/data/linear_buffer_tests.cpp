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

    void TearDown() override
    {
        buffer.reset();
    }

    std::unique_ptr<LinearBuffer> buffer;
};

// Basic construction and initial state tests
TEST_F(LinearBufferTest, InitialState)
{
    EXPECT_EQ(buffer->capacity(), 100);
    EXPECT_EQ(buffer->available_data(), 0);
    EXPECT_EQ(buffer->available_space(), 100);
    EXPECT_TRUE(buffer->is_empty());
    EXPECT_FALSE(buffer->is_full());
}

TEST_F(LinearBufferTest, ConstructorWithZeroSize)
{
    LinearBuffer zero_buffer(0);
    EXPECT_EQ(zero_buffer.capacity(), 0);
    EXPECT_TRUE(zero_buffer.is_empty());
    EXPECT_TRUE(zero_buffer.is_full());
}

// Push operation tests
TEST_F(LinearBufferTest, PushValidData)
{
    const char* test_data = "Hello";
    std::size_t data_size = 5;

    EXPECT_TRUE(buffer->push(test_data, data_size));
    EXPECT_EQ(buffer->available_data(), data_size);
    EXPECT_EQ(buffer->available_space(), 100 - data_size);
    EXPECT_FALSE(buffer->is_empty());
}

TEST_F(LinearBufferTest, PushNullPointer)
{
    EXPECT_FALSE(buffer->push(nullptr, 5));
    EXPECT_TRUE(buffer->is_empty());
}

TEST_F(LinearBufferTest, PushZeroSize)
{
    const char* test_data = "Hello";
    EXPECT_FALSE(buffer->push(test_data, 0));
    EXPECT_TRUE(buffer->is_empty());
}

TEST_F(LinearBufferTest, PushExceedsCapacity)
{
    // Try to push more data than buffer capacity
    std::string large_data(150, 'X');
    EXPECT_FALSE(buffer->push(large_data.data(), large_data.size()));
    EXPECT_TRUE(buffer->is_empty());
}

TEST_F(LinearBufferTest, PushUntilFull)
{
    std::string data(100, 'A');
    EXPECT_TRUE(buffer->push(data.data(), data.size()));
    EXPECT_TRUE(buffer->is_full());
    EXPECT_EQ(buffer->available_space(), 0);

    // Try to push one more byte
    char extra = 'B';
    EXPECT_FALSE(buffer->push(&extra, 1));
}

TEST_F(LinearBufferTest, PushMultipleChunks)
{
    const char* chunk1 = "Hello";
    const char* chunk2 = " ";
    const char* chunk3 = "World";

    EXPECT_TRUE(buffer->push(chunk1, 5));
    EXPECT_TRUE(buffer->push(chunk2, 1));
    EXPECT_TRUE(buffer->push(chunk3, 5));

    EXPECT_EQ(buffer->available_data(), 11);
    EXPECT_EQ(buffer->available_space(), 89);
}

// Peek operation tests
TEST_F(LinearBufferTest, PeekEmptyBuffer)
{
    std::size_t size_out;
    const void* data = buffer->peek(size_out);

    EXPECT_EQ(data, nullptr);
    EXPECT_EQ(size_out, 0);
}

TEST_F(LinearBufferTest, PeekWithData)
{
    const char* test_data = "Hello World";
    std::size_t data_size = 11;

    buffer->push(test_data, data_size);

    std::size_t size_out;
    const void* data = buffer->peek(size_out);

    EXPECT_NE(data, nullptr);
    EXPECT_EQ(size_out, data_size);

    // Verify the data content
    const char* char_data = static_cast<const char*>(data);
    EXPECT_EQ(std::strncmp(char_data, test_data, data_size), 0);
}

TEST_F(LinearBufferTest, PeekContiguousData)
{
    // Push multiple chunks and verify they're accessible as contiguous memory
    buffer->push("Hello", 5);
    buffer->push(" ", 1);
    buffer->push("World", 5);

    std::size_t size_out;
    const void* data = buffer->peek(size_out);

    EXPECT_NE(data, nullptr);
    EXPECT_EQ(size_out, 11);

    // Verify all data is contiguous
    const char* char_data = static_cast<const char*>(data);
    std::string result(char_data, size_out);
    EXPECT_EQ(result, "Hello World");
}

// Consume operation tests
TEST_F(LinearBufferTest, ConsumeFromEmptyBuffer)
{
    EXPECT_FALSE(buffer->consume(1));
    EXPECT_TRUE(buffer->is_empty());
}

TEST_F(LinearBufferTest, ConsumeMoreThanAvailable)
{
    buffer->push("Hello", 5);
    EXPECT_FALSE(buffer->consume(10));
    EXPECT_EQ(buffer->available_data(), 5); // Data should remain unchanged
}

TEST_F(LinearBufferTest, ConsumePartialData)
{
    buffer->push("Hello World", 11);

    EXPECT_TRUE(buffer->consume(6)); // Consume "Hello "
    EXPECT_EQ(buffer->available_data(), 5);

    // Verify remaining data
    std::size_t size_out;
    const void* data = buffer->peek(size_out);
    const char* char_data = static_cast<const char*>(data);
    
    EXPECT_EQ(size_out, 5);
    EXPECT_EQ(std::strncmp(char_data, "World", 5), 0);
}

TEST_F(LinearBufferTest, ConsumeAllData)
{
    buffer->push("Hello", 5);
    EXPECT_TRUE(buffer->consume(5));
    EXPECT_TRUE(buffer->is_empty());
    EXPECT_EQ(buffer->available_space(), 100); // Should have full capacity again
}

// Reset behavior tests
TEST_F(LinearBufferTest, ResetOnCompleteConsumption)
{
    // Fill buffer partially
    buffer->push("Test", 4);
    EXPECT_EQ(buffer->available_data(), 4);

    // Consume all data
    EXPECT_TRUE(buffer->consume(4));
    EXPECT_TRUE(buffer->is_empty());

    // After reset, we should be able to use full capacity again
    std::string large_data(100, 'X');
    EXPECT_TRUE(buffer->push(large_data.data(), large_data.size()));
    EXPECT_TRUE(buffer->is_full());
}

TEST_F(LinearBufferTest, NoResetOnPartialConsumption)
{
    buffer->push("Hello World", 11);
    buffer->consume(6); // Consume "Hello "

    // Should still have "World" available
    std::size_t size_out;
    const void* data = buffer->peek(size_out);
    EXPECT_EQ(size_out, 5);
    
    const char* char_data = static_cast<const char*>(data);
    EXPECT_EQ(std::strncmp(char_data, "World", 5), 0);
}

// Clear operation tests
TEST_F(LinearBufferTest, ClearBuffer)
{
    buffer->push("Hello World", 11);
    EXPECT_FALSE(buffer->is_empty());

    buffer->clear();
    EXPECT_TRUE(buffer->is_empty());
    EXPECT_EQ(buffer->available_space(), 100);

    // Should be able to use full capacity after clear
    std::string large_data(100, 'X');
    EXPECT_TRUE(buffer->push(large_data.data(), large_data.size()));
}

// Edge case tests
TEST_F(LinearBufferTest, InsufficientContiguousSpace)
{
    // Fill buffer to near capacity
    std::string data1(90, 'A');
    buffer->push(data1.data(), data1.size());

    // Consume some from the beginning
    buffer->consume(50);

    // Now we have 40 bytes of data and 60 bytes free, but only 10 bytes contiguous at the end
    // Try to push 20 bytes (should fail due to lack of contiguous space)
    std::string data2(20, 'B');
    EXPECT_FALSE(buffer->push(data2.data(), data2.size()));

    // Should still have the original remaining data
    EXPECT_EQ(buffer->available_data(), 40);
}

TEST_F(LinearBufferTest, PushAfterReset)
{
    // Use buffer, consume all, then reuse
    buffer->push("First", 5);
    buffer->consume(5);
    EXPECT_TRUE(buffer->is_empty());

    // After reset, should be able to push new data from the beginning
    buffer->push("Second", 6);
    
    std::size_t size_out;
    const void* data = buffer->peek(size_out);
    const char* char_data = static_cast<const char*>(data);
    
    EXPECT_EQ(size_out, 6);
    EXPECT_EQ(std::strncmp(char_data, "Second", 6), 0);
}

// Stress test with multiple operations
TEST_F(LinearBufferTest, MultipleOperationSequence)
{
    // Push, peek, consume in various combinations
    buffer->push("ABC", 3);
    buffer->push("DEF", 3);
    
    std::size_t size_out;
    const void* data = buffer->peek(size_out);
    EXPECT_EQ(size_out, 6);
    
    buffer->consume(2); // Consume "AB"
    data = buffer->peek(size_out);
    EXPECT_EQ(size_out, 4);
    
    const char* char_data = static_cast<const char*>(data);
    EXPECT_EQ(std::strncmp(char_data, "CDEF", 4), 0);
    
    buffer->push("GHI", 3);
    data = buffer->peek(size_out);
    EXPECT_EQ(size_out, 7);
    
    char_data = static_cast<const char*>(data);
    EXPECT_EQ(std::strncmp(char_data, "CDEFGHI", 7), 0);
}
