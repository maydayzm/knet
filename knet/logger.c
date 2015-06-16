#include <stdarg.h>
#include "logger.h"
#include "misc.h"

logger_t* global_logger = 0;

struct _logger_t {
    FILE*          fd;
    logger_level_e level;
    logger_mode_e  mode;
    lock_t*        lock;
};

logger_t* logger_create(const char* path, logger_level_e level, logger_mode_e mode) {
    char temp[PATH_MAX] = {0};
    logger_t* logger = create(logger_t);
    memset(logger, 0, sizeof(logger_t));
    logger->mode  = mode;
    logger->level = level;
    logger->lock  = lock_create();
    assert(logger->lock);
    if (!path) {
        /* ��־�����ڵ�ǰĿ¼ */
        path = path_getcwd(temp, sizeof(temp));
        strcat(temp, "/knet.log");
    }
    if (mode & logger_mode_file) {
        assert(path);
        if (mode & logger_mode_override) {
            /* �򿪲���� */
            logger->fd = fopen(path, "w+");
        } else {
            /* ���ӵ�ԭ����־ */
            logger->fd = fopen(path, "a+");
        }
        if (!logger->fd) {
            goto fail_return;
        }
    }
    return logger;
fail_return:
    destroy(logger);
    return 0;
}

void logger_destroy(logger_t* logger) {
    assert(logger);
    if (logger->fd) {
        fclose(logger->fd);
    }
    lock_destroy(logger->lock);
    destroy(logger);
}

int logger_write(logger_t* logger, logger_level_e level, const char* format, ...) {
    char buffer[64] = {0};
    int  bytes = 0;
    static const char* logger_level_name[] = { 0, "VERB", "INFO", "WARN", "ERRO", "FATA" };
    va_list arg_ptr;
    assert(logger);
    assert(format);
    if (logger->level > level) {
        /* ��־�ȼ����� */
        return error_ok;
    }
    va_start(arg_ptr, format);
    time_get_string(buffer, sizeof(buffer));
    if (logger->mode & logger_mode_file) {
        /* д����־�ļ� */
        lock_lock(logger->lock);
        fprintf(logger->fd, "[%s][%s]", logger_level_name[level], buffer);
        bytes = vfprintf(logger->fd, format, arg_ptr);
        if (bytes <= 0) {
            lock_unlock(logger->lock);
            return error_logger_write;
        }
        fprintf(logger->fd, "\n");
        if (logger->mode & logger_mode_flush) {
            /* ����д�� */
            fflush(logger->fd);
        }
        lock_unlock(logger->lock);
    }
    if (logger->mode & logger_mode_console) {
        /* д��stderr */
        lock_lock(logger->lock);
        fprintf(stderr, "[%s][%s]", logger_level_name[level], buffer);
        vfprintf(stderr, format, arg_ptr);
        fprintf(stderr, "\n");
        if (logger->mode & logger_mode_flush) {
            /* ����д�� */
            fflush(stderr);
        }
        lock_unlock(logger->lock);
    }
    va_end(arg_ptr);  
    return error_ok;
}