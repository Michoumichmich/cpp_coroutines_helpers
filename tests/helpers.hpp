#pragma once

#include <gtest/gtest.h>
#include <stdexcept>

#define EXPECT_THROW_RUNTIME_ERROR_STREQ(stmt, string) \
    EXPECT_THROW(                                      \
        try{                                           \
              stmt                                     \
        } catch (std::exception &e) {                  \
            EXPECT_STREQ(string, e.what());            \
            throw;                                     \
        }, std::runtime_error)
