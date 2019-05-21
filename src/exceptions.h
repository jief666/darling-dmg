#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H
#include <stdexcept>

// Cannot resolve given file/directory path.
class file_not_found_error : public std::runtime_error
{
public:
	//	using std::runtime_error::runtime_error; // ctor inheritance is not supported by all compiler.
	file_not_found_error(const std::string& what_arg) : std::runtime_error(what_arg) {};
};

// Used for fatal errors. Indicates a bug or severe data corruption.
class io_error : public std::runtime_error
{
public:
	//	using std::runtime_error::runtime_error; // ctor inheritance is not supported by all compiler.
	io_error(const std::string& what_arg) : std::runtime_error(what_arg) {};
};

// Used to indicate non-existent xattr
class no_data_error : public std::exception
{
public:
	virtual const char* what() const noexcept override { return "No data available"; }
};

// Used when something unexpected or unknown is encountered. May also indicate data corruption.
class function_not_implemented_error : public std::runtime_error
{
public:
	//	using std::runtime_error::runtime_error; // ctor inheritance is not supported by all compiler.
	function_not_implemented_error(const std::string& what_arg) : std::runtime_error(what_arg) {};
};


// Used to indicate non-existent xattr
class attribute_not_found_error : public std::exception
{
public:
	virtual const char* what() const noexcept override { return "Attribute not found (93)"; }
};

// Used to indicate non-existent xattr on a directory
class operation_not_permitted_error : public std::exception
{
public:
	virtual const char* what() const noexcept override { return "Operation not permitted (1)"; }
};

#endif
