
enum ser_mode {
    SER_MODE_READ = 0,
    SER_MODE_WRITE = 1,
};

// A serializer can be used to read _or_ write binary files. It contains an
// internal buffer to it will store or load the bytes pass to it trough the
// serialize method.
// To write, create a serializer with the constructor, call serialize on your
// data to populate the internal buffer, then call save_to_file.
// To read, create a serializer with Serializer::create_from_file specifing
// a filepath to load and call serialize() on your data to deserialize from the
// internal buffer.
struct serializer_t
{
    ser_mode mode;
    int      buffer_capacity;
    int      buffer_used; // maybe mis-named, also represent the read position.
    uint8_t* buffer;

    serializer_t(size_t capacity = 1024)
    {
        this->mode = SER_MODE_WRITE;
        this->buffer_capacity = capacity;
        this->buffer = (uint8_t*) malloc(ser.buffer_capacity);
    }

    static serializer_t create_from_file(const char* filepath)
    {
        span_t file_data = load_entire_file(filepath);
        serializer_t ser = {};
        ser.mode = SER_MODE_READ;
        ser.buffer = file_data.ptr;
        ser.buffer_capacity = file_data.size;
        ser.buffer_used = 0; // just to be explicit
        return ser;
    }

    void serializer_t::save_to_file(const char* filepath)
    {
        FILE* f = fopen(, "wb");
        fwrite(ser.buffer, ser.buffer_used, 1, f);
        fclose(f);
    }

    void serialize(serializer_t* ser, void* ptr, size_t size)
    {
        if (ser->mode == SER_MODE_READ) {
            size_t remaining_bytes = ser->buffer_capacity - ser->buffer_used;
            if (remaining_bytes < size) {
                LOG_ERROR("Serializer tried to read past end of buffer");
                exit(-1);
            } 
            memcpy(ptr, ser->buffer + ser->buffer_used, size);
            ser->buffer_used += size;
        } else {
            size_t remaining_capacity = ser->buffer_capacity - ser->buffer_used;
            if (remaining_capacity < size) {
                LOG_ERROR("Serializer out of memory");
                exit(-1);
            } 
            memcpy(ser->buffer + ser->buffer_used, ptr, size);
            ser->buffer_used += size;
        }
    }
};


