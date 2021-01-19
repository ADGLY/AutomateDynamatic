#pragma once

typedef enum
{
    ERR_NONE,
    ERR_MEM,
    ERR_IO,
    ERR_REGEX,
    ERR_NAME_TOO_LONG,
    ERR_NULL,
    ERR_PATH
} auto_error_t;

#define CHECK_PARAM(param)                                              \
    do                                                                  \
    {                                                                   \
        if ((param) == NULL)                                            \
        {                                                               \
            fprintf(stderr, "%s:%d | Bad param\n", __FILE__, __LINE__); \
            return ERR_NULL;                                            \
        }                                                               \
    } while (0);

#define CHECK_NULL(arg, err, msg)                                                \
    do                                                                           \
    {                                                                            \
        if ((arg) == NULL)                                                       \
        {                                                                        \
            fprintf(stderr, "%s:%d | %s : %s\n", __FILE__, __LINE__, #err, msg); \
            return err;                                                          \
        }                                                                        \
    } while (0);

#define CHECK_CALL(err, msg)                                          \
    do                                                                \
    {                                                                 \
        auto_error_t temp_err = (err);                                \
        if ((temp_err) != ERR_NONE)                                   \
        {                                                             \
            fprintf(stderr, "%s:%d | %s\n", __FILE__, __LINE__, msg); \
            return temp_err;                                          \
        }                                                             \
    } while (0);

#define CHECK_CALL_DO(err, msg, what_to_do)                           \
    do                                                                \
    {                                                                 \
        auto_error_t temp_err = (err);                                \
        if ((temp_err) != ERR_NONE)                                   \
        {                                                             \
            fprintf(stderr, "%s:%d | %s\n", __FILE__, __LINE__, msg); \
            what_to_do;                                               \
            return temp_err;                                          \
        }                                                             \
    } while (0);

#define CHECK_COND(cond, err, msg)                                             \
    do                                                                         \
    {                                                                          \
        if ((cond))                                                            \
        {                                                                      \
            fprintf(stderr, "%s:%d | %s:%s\n", __FILE__, __LINE__, #err, msg); \
            return err;                                                        \
        }                                                                      \
    } while (0);

#define CHECK_COND_DO(cond, err, msg, what_to_do)                              \
    do                                                                         \
    {                                                                          \
        if ((cond))                                                            \
        {                                                                      \
            fprintf(stderr, "%s:%d | %s:%s\n", __FILE__, __LINE__, #err, msg); \
            what_to_do;                                                        \
            return err;                                                        \
        }                                                                      \
    } while (0);

#define CHECK_LENGTH(length, max)                                             \
    do                                                                        \
    {                                                                         \
        if ((length) >= (max))                                                \
        {                                                                     \
            fprintf(stderr, "%s:%d | Name too long !\n", __FILE__, __LINE__); \
            return ERR_NAME_TOO_LONG;                                         \
        }                                                                     \
    } while (0);
