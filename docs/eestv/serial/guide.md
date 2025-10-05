# Serialization Usage Guide

This document shows how to use the EESTV serialization library.

## Basic Usage Flow

```mermaid
flowchart LR
    A[Create Storage] --> B[Create Serializer/Deserializer]
    B --> C[Define serialize method in your type]
    C --> D[Use operator& to serialize fields]
    
    style A fill:#e1f5ff
    style B fill:#e1f5ff
    style C fill:#fff4e1
    style D fill:#e8f5e9
```

## How to Make Your Type Serializable

```mermaid
flowchart TD
    Start[I want to serialize MyType] --> Choice{Choose approach}
    
    Choice -->|Option 1| Member[Member function]
    Choice -->|Option 2| Free[Free function]
    
    Member --> Done[✓ Type is serializable]
    Free --> Done
    
    style Start fill:#e3f2fd
    style Member fill:#c8e6c9
    style Free fill:#c8e6c9
    style Done fill:#a5d6a7
```

### Option 1: Member Function
```cpp
struct MyType {
    int x, y;
    
    template<typename Archive>
    void serialize(Archive& ar) {
        ar & x & y;
    }
};
```

### Option 2: Free Function
```cpp
struct MyType {
    int x, y;
};

template<typename Archive>
void serialize(MyType& obj, Archive& ar) {
    ar & obj.x & obj.y;
}
```

## Complete Example

```mermaid
sequenceDiagram
    participant User
    participant Storage as Storage Backend
    participant Serializer
    participant MyData as Your Data Object
    
    User->>Storage: 1. Create buffer/storage
    User->>Serializer: 2. Create Serializer(storage)
    User->>MyData: 3. Create data object
    User->>Serializer: 4. serializer & data
    Serializer->>MyData: 5. Call data.serialize(serializer)
    MyData->>Serializer: 6. serializer & field1 & field2
    Serializer->>Storage: 7. Write bytes to storage
    Storage-->>User: 8. Data serialized!
    
    Note over User,Storage: Deserialization is the reverse process
```
