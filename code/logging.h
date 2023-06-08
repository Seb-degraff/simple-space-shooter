#pragma once

#define STRINGIFY(a) STRINGIFY_INTERNAL(a)
#define STRINGIFY_INTERNAL(a) #a

#define ANSI_RED "\e[31m"
#define ANSI_RESET "\e[0m"  
#define ANSI_GRAY "\e[90m"

#define LOG(message) printf(message ANSI_GRAY " (" __FILE__ ":" STRINGIFY(__LINE__) ")\n" ANSI_RESET)
#define LOG_ERROR(message) printf("\e[31m" "ERROR: " message ANSI_GRAY " (" __FILE__ ":" STRINGIFY(__LINE__) ")\n" ANSI_RESET)
