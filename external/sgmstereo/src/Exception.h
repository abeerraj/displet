#ifndef REVLIB_EXCEPTION_H
#define REVLIB_EXCEPTION_H

#include <string>

namespace rev {
    
class Exception {
public:
    Exception(const std::string functionName = "", const std::string message = "")
        : functionName_(functionName), message_(message) {}
        
    std::string functionName() const { return functionName_; }
    std::string message() const { return message_; }
        
private:
    std::string functionName_;
    std::string message_;
};
    
}

#endif
