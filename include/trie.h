/*
 * Copyright (c) 2009, Jianing Yang<jianingy.yang@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * The names of its contributors may not be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY detrox@gmail.com ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL detrox@gmail.com BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#ifndef TRIE_H_
#define TRIE_H_

#include <map>
#include <vector>
#include <cstdlib>
#include <stdexcept>

#define BEGIN_TRIE_NAMESPACE namespace trie {
#define END_TRIE_NAMESPACE }

BEGIN_TRIE_NAMESPACE

/// Represents a value in double-array.
typedef int32_t value_type;

/// Represents a size or an index value for accessing states in double-array.
typedef int32_t size_type;

/// Represents a transition character.
typedef int32_t char_type;

/// Represents a trie type.
enum trie_type {
    UNKNOW = 0,   /**< Unknow. */
    SINGLE_TRIE,  /**< Tail Trie. */
    DOUBLE_TRIE   /**< Two Trie. */
};

/**
 * Represents trie archive error.
 *
 * This exception will be threw when there are errors about
 * trie's index file.
 */
class bad_trie_archive: public std::runtime_error {
  public:
    /**
     * Constructs a bad_trie_archive.
     *
     * @param s Detail description.
     */
    explicit bad_trie_archive(const char *s):std::runtime_error(s) {}
};

/**
 * Represents source text error.
 *
 * This exception will be threw when there are errors about
 * a formatted text file.
 */
class bad_trie_source: public std::runtime_error {
  public:
    /**
     * Constructs a bad_trie_source.
     *
     * @param s Detail description.
     */
    explicit bad_trie_source(const char *s):std::runtime_error(s) {}
};

/**
 * Represents a key to access trie.
 *
 * This class can convert other data format to trie's key.
 */
class key_type {
  public:
    /// Charset size
    static const char_type kCharsetSize = 257;

    /// Terminator character (character not in charset).
    static const char_type kTerminator = kCharsetSize;

    /// Constructs an empty key_type.
    key_type()
        :cstr_(NULL), cstr_capacity_(0),
         data_(NULL), data_capacity_(0),
         length_(0)
    {}

    /**
     * Constructs a key_type from a c-style data.
     *
     * @param data Pointer to the c-style data.
     * @param length Length of the c-style data.
     */
    explicit key_type(const char *data, size_t length)
        :cstr_(NULL), cstr_capacity_(0),
         data_(NULL), data_capacity_(0),
         length_(0)
    {
        assign(data, length);
    }

    /**
     * Constructs a copy from a key.
     *
     * @param key The key
     */
    explicit key_type(const key_type &key)
        :cstr_(NULL), cstr_capacity_(0),
         data_(NULL), data_capacity_(0),
         length_(0)
    {
        assign(key.data(), key.length());
    }

    /**
     * Copies from a key.
     *
     * @param rhs The key
     */
    const key_type &operator=(const key_type &rhs)
    {
        assign(rhs.data(), rhs.length());
        return *this;
    }

    /**
     * Destruct a key_type.
     */
    ~key_type()
    {
        free(data_);
        free(cstr_);
        data_capacity_ = 0;
        cstr_capacity_ = 0;
    }

    /**
     * Returns a const pointer to internal data of a key_type.
     */
    const char_type *data() const
    {
        return data_;
    }

    /**
     * Returns length of a key_type.
     */
    size_t length() const
    {
        return length_;
    }

    /// Converts a char to char_type.
    static char_type char_in(const char ch)
    {
        return static_cast<unsigned char>(ch + 1);
    }

    /// Converts a char_type to char.
    static char char_out(char_type ch)
    {
        return static_cast<char>(ch - 1);
    }

    /// Appends a char_type to the end of a key_type.
    void push(char_type ch)
    {
        if (length_ + 1 >= data_capacity_)
            resize_data(1);
        data_[length_++] = ch;
        data_[length_] = kTerminator;
    }

    /**
     * Removes a char_type from the end of a key_type and returns its
     * value.
     */
    char_type pop()
    {
        char_type ch;
        ch = data_[length_--];
        data_[length_] = kTerminator;
        return ch;
    }

    /// Clears a key_type.
    void clear()
    {
        data_[0] = kTerminator;
        length_ = 0;
    }

    /**
     * Returns a key_type as c-style string.
     *
     * @return Pointer to buffer of the c-style string.
     */
    const char *c_str() const
    {
        size_t i;
        if (cstr_capacity_ < data_capacity_)
            resize_cstr();
        for (i = 0; data_[i] != kTerminator ; i++)
            cstr_[i] = char_out(data_[i]);
        cstr_[i] = '\0';
        return cstr_;
    }

    /**
     * Updates a key_type with a c-style data.
     *
     * @param data Pointer to the data.
     * @param length Length of the data.
     */
    void assign(const char *data, size_t length)
    {
        size_t i;
        if (length + 1 >= data_capacity_)
            resize_data(length);
        for (i = 0; i < length; i++)
            data_[i] = char_in(data[i]);
        data_[i] = kTerminator;
        length_ = length;
    }

    /**
     * Updates a key_type with a char_type data.
     *
     * @param data Pointer to the data.
     * @param length Length of the data.
     */
    void assign(const char_type *data, size_t length)
    {
        size_t i;
        if (length + 1 >= data_capacity_)
            resize_data(length);
        for (i = 0; i < length; i++)
            data_[i] = data[i];
        data_[i] = kTerminator;
        length_ = length;
    }

  protected:
    /**
     * Resizes the internal data buffer of a key_type.
     *
     * @param size Expected size.
     */
    void resize_data(size_t size)
    {
        size_t nsize = (data_capacity_ + size + 1) * 2;
        data_ = static_cast<char_type *>
            (realloc(data_, nsize * sizeof(char_type)));
        data_capacity_ = nsize;
    }

    /**
     * Resizes the buffer which is used to convert key_type to c-style
     * string.
     */
    void resize_cstr() const
    {
        cstr_ = static_cast<char *>
            (realloc(cstr_, data_capacity_ * sizeof(char)));
        cstr_capacity_ = data_capacity_;
    }

  private:
    /// a C-style buffer for converting.
    mutable char *cstr_;

    /// Size of cstr_.
    mutable size_t cstr_capacity_;

    /// Internal data buffer.
    char_type *data_;

    /// Size of data_.
    size_t data_capacity_;

    /// Length of data_.
    size_t length_;
};

/// Represents a result set for prefix_search.
typedef std::vector<std::pair<key_type, value_type> > result_type;

/**
 * Represents an interface for different trie structure.
 */
class trie_interface {
  public:
    /// Constructs a trie_interface.
    trie_interface() {}

    /**
     * Constructs a trie_interface with a specified state size.
     **
     * @param size The initial size of states.
     */
    explicit trie_interface(size_t size) {}

    /**
     * Constructs a trie_interface from a archive file.
     *
     * @param filename The archive filename.
     */
    explicit trie_interface(const char *filename) {}

    /**
     * Stores a value_type into trie using a key_type as key.
     *
     * @param key The key.
     * @param value The value_type.
     */
    virtual void insert(const key_type &key, const value_type &value) = 0;

    /**
     * Retrieves a value_type from trie using a key_type as key.
     *
     * @param key The key.
     * @param value The value_type.
     * @return true if found.
     */
    virtual bool search(const key_type &key, value_type *value) const = 0;

    /**
     * Stores a value_type into trie using a c-style string as key
     *
     * @param inputs Buffer of the key.
     * @param length Length of the key buffer.
     * @param value The value_type.
     */
    virtual void insert(const char *inputs, size_t length,
                        value_type value);

    /**
     * Retrieves a value_type from trie using a c-style string as key
     *
     * @param inputs Buffer of the key.
     * @param length Length of the key buffer.
     * @param value The value_type.
     * @return true if found.
     */
    virtual bool search(const char *inputs, size_t length,
                        value_type *value) const;

    /**
     * Retrieves all key-value pairs match given prefix.
     *
     * @param key The prefix.
     * @param[out] result Result set contains the existing keys.
     * @return The number of elements in the result set.
     */
    virtual size_t prefix_search(const key_type &key,
                                 result_type *result) const = 0;
    /**
     * Builds a trie archive.
     *
     * @param filename Filename of the archive.
     * @param verbose Display detail information while building
     *                if sets to true.
     */
    virtual void build(const char *filename, bool verbose = false) = 0;

    /**
     * Updates a trie from a formatted text file.
     *
     * @param source Filename of the text file.
     * @param verbose Display detail information while reading
     *                if it sets to true.
     */
    virtual void read_from_text(const char *source, bool verbose = false);

    /**
     * Destruct a trie_interface.
     */
    virtual ~trie_interface() = 0;
};

/**
 * Creates an empty trie.
 *
 * @param trie_type The type of the trie to be created.
 * @param size The initial size of the trie. This is not an accurate value
 *             but a suggestion. Please increase or decrease this value
 *             according to the size of your data.
 */
trie_interface *create_trie(trie_type type = DOUBLE_TRIE, size_t size = 4096);

/**
 * Creates a trie from a trie archive.
 *
 * @param archive The filename of the archive.
 */
trie_interface *create_trie(const char *archive);

END_TRIE_NAMESPACE

#endif  // TRIE_H_

// vim: ts=4 sw=4 ai et
