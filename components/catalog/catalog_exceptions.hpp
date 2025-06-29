#pragma once

#include <exception>
#include <string>

namespace catalog {
    class iceberg_exception : public std::exception {
    protected:
        std::string message_;

    public:
        explicit iceberg_exception(const std::string& message)
            : message_(message) {}

        const char* what() const noexcept override { return message_.c_str(); }
    };

    class no_such_table_exception : public iceberg_exception {
    public:
        explicit no_such_table_exception(const std::string& message)
            : iceberg_exception(message) {}
    };

    class no_such_namespace_exception : public iceberg_exception {
    public:
        explicit no_such_namespace_exception(const std::string& message)
            : iceberg_exception(message) {}
    };

    class already_exists_exception : public iceberg_exception {
    public:
        explicit already_exists_exception(const std::string& message)
            : iceberg_exception(message) {}
    };

    class commit_failed_exception : public iceberg_exception {
    public:
        explicit commit_failed_exception(const std::string& message)
            : iceberg_exception(message) {}
    };

    class validation_exception : public iceberg_exception {
    public:
        explicit validation_exception(const std::string& message)
            : iceberg_exception(message) {}
    };

    class not_supported_exception : public iceberg_exception {
    public:
        explicit not_supported_exception(const std::string& message)
            : iceberg_exception(message) {}
    };

} // namespace catalog