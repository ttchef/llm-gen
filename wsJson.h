
#ifndef WS_JSON_H
#define WS_JSON_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#define WS_JSON_MAX_KEY_SIZE 64 
#define WS_JSON_MAX_VALUE_SIZE 256

#define WS_ERROR -1
#define WS_OK 0

/* 
 *  Allocators  
 *  Redefine with own ones to use custom allocator
 */
#ifndef WS_JSON_MALLOC 
    #define WS_JSON_MALLOC(size) malloc(size)
#endif

#ifndef WS_JSON_REALLOC
    #define WS_JSON_REALLOC(ptr, size) realloc(ptr, size)
#endif 

#ifndef WS_JSON_CALLOC 
    #define WS_JSON_CALLOC(n, size) calloc(n, size)
#endif 

#ifndef WS_JSON_FREE 
    #define WS_JSON_FREE(ptr) free(ptr)
#endif

typedef enum wsJsonType {
    WS_JSON_STRING,
    WS_JSON_NUMBER,
    WS_JSON_OBJECT,
    WS_JSON_BOOL,
    WS_JSON_ARRAY,
    WS_JSON_NULL
} wsJsonType;

typedef struct wsJson {
    char key[WS_JSON_MAX_KEY_SIZE];
    wsJsonType type;
    union {
        char* stringValue;
        double numberValue;
        bool boolValue;
        struct {
            struct wsJson** children;
            int32_t childCount;
            int32_t childCapacity;
        } object;
        struct {
            struct wsJson** elements;
            int32_t elementCount;
            int32_t elementCapacity;
        } array;
    };
} wsJson;

// Create functions
wsJson* wsJsonInitObject(const char* key);
wsJson* wsJsonInitString(const char* key, const char* val);
wsJson* wsJsonInitNumber(const char* key, double val);
wsJson* wsJsonInitBool(const char* key, bool val);
wsJson* wsJsonInitArray(const char* key);
wsJson* wsJsonInitNull(const char* key);

// Adds a new child to the json object
void wsJsonAddField(wsJson* parent, wsJson* child);

// Adds an element to a json array
void wsJsonAddElement(wsJson* array, wsJson* element);

// String conversions
int32_t wsJsonToString(wsJson* obj, char* out, size_t size);
int32_t wsJsonToStringPretty(wsJson* obj, char* out, size_t size);
wsJson* wsStringToJson(const char** string);

// Get Values 
wsJson* wsJsonGet(wsJson* obj, const char* key);
char* wsJsonGetString(wsJson* obj, const char* key);
int32_t wsJsonGetStringEx(wsJson* obj, const char* key, char* out, size_t size);
double wsJsonGetNumber(wsJson* obj, const char* key);
bool wsJsonGetBool(wsJson* obj, const char* key);

// Array Getters
int32_t wsJsonGetArrayLen(wsJson* obj, const char* key);
wsJson* wsJsonGetArrayAt(wsJson* obj, const char* key, int32_t index);

// Setter Explicit Functions (if object is null it wont set)
int32_t wsJsonSetStringExplicit(wsJson* obj, const char* key, const char* val);
int32_t wsJsonSetNumberExplicit(wsJson* obj, const char* key, double val);
int32_t wsJsonSetBoolExplicit(wsJson* obj, const char* key, bool val);

// Setter functions (if object is null it will change it to the specified type)
int32_t wsJsonSetString(wsJson* obj, const char* key, const char* val);
int32_t wsJsonSetNumber(wsJson* obj, const char* key, double val);
int32_t wsJsonSetBool(wsJson* obj, const char* key, bool val);
int32_t wsJsonSetElement(wsJson* obj, const char* key, int32_t index, wsJson* element);

// Null conversions
int32_t wsJsonSetNullToObject(wsJson* obj, const char* key, wsJson* objects);
int32_t wsJsonSetNullToString(wsJson* obj, const char* key, const char* val);
int32_t wsJsonSetNullToNumber(wsJson* obj, const char* key, double val);
int32_t wsJsonSetNullToBool(wsJson* obj, const char* key, bool val);
int32_t wsJsonSetNullToArray(wsJson* obj, const char* key, wsJson* array);

// Goes recursive trough the json tree and frees everything
void wsJsonFree(wsJson* obj);

#ifndef WS_JSON_NO_MACROS
    #define wsJsonAddString(parent, key, val) (wsJsonAddField(parent, wsJsonInitString(key, val)))
    #define wsJsonAddNumber(parent, key, val) (wsJsonAddField(parent, wsJsonInitNumber(key, val)))
    #define wsJsonAddBool(parent, key, val) (wsJsonAddField(parent, wsJsonInitBool(key, val)))
    #define wsJsonAddNull(parent, key) (wsJsonAddField(parent, wsJsonInitNull(key)))
#endif // WS_JSON_MACROS

/* Log */
#include <threads.h>
#include <stdbool.h>
#include <stdarg.h>

typedef enum wsJsonLogLevel {
    WS_JSON_LOG_LEVEL_ERROR,
    WS_JSON_LOG_LEVEL_WARNING,
    WS_JSON_LOG_LEVEL_DEBUG,
    WS_JSON_LOG_LEVEL_INFO,
    WS_JSON_LOG_LEVEL_API_DUMP,
} wsJsonLogLevel;

extern _Thread_local int32_t _wsJsonLogLevel;

const char* _wsJsonErrorLogLevelToString(wsJsonLogLevel level);
void wsJsonSetLogLevel(int32_t level);

#ifdef WS_JSON_IMPLEMENTATION

#include <ctype.h>
#include <string.h>
#include <stdio.h>

/* Log */ 
void _wsJsonLogImpl(int32_t level, const char* file, const char* func, int32_t line, const char* msg, ...) {
    if (level <= _wsJsonLogLevel) {
        va_list vlist;
        va_start(vlist, msg);
        
        char formatedMsg[256];
        vsnprintf(formatedMsg, sizeof(formatedMsg), msg, vlist);
        fprintf(stderr, "[%s] %s:%d (%s): %s\n", _wsJsonErrorLogLevelToString(level), file, line, func, formatedMsg);

        va_end(vlist);
    }
}

#define WS_JSON_LOG_API_DUMP(msg, ...) \
    _wsJsonLogImpl(WS_JSON_LOG_LEVEL_API_DUMP, __FILE__, __func__, __LINE__, msg, ##__VA_ARGS__)

#define WS_JSON_LOG_INFO(msg, ...) \
    _wsJsonLogImpl(WS_JSON_LOG_LEVEL_INFO, __FILE__, __func__, __LINE__, msg, ##__VA_ARGS__)

#define WS_JSON_LOG_DEBUG(msg, ...) \
    _wsJsonLogImpl(WS_JSON_LOG_LEVEL_DEBUG, __FILE__, __func__, __LINE__, msg, ##__VA_ARGS__)

#define WS_JSON_LOG_WARNING(msg, ...) \
    _wsJsonLogImpl(WS_JSON_LOG_LEVEL_WARNING, __FILE__, __func__, __LINE__, msg, ##__VA_ARGS__)

#define WS_JSON_LOG_ERROR(msg, ...) \
    _wsJsonLogImpl(WS_JSON_LOG_LEVEL_ERROR, __FILE__, __func__, __LINE__, msg, ##__VA_ARGS__)


_Thread_local int32_t _wsJsonLogLevel = WS_JSON_LOG_LEVEL_WARNING;

const char* _wsJsonErrorLogLevelToString(wsJsonLogLevel level) {
    switch (level) {
        case WS_JSON_LOG_LEVEL_API_DUMP:      return "WS_JSON_API_DUMP";
        case WS_JSON_LOG_LEVEL_INFO:          return "WS_JSON_INFO";
        case WS_JSON_LOG_LEVEL_DEBUG:         return "WS_JSON_DEBUG";
        case WS_JSON_LOG_LEVEL_WARNING:       return "WS_JSON_WARNING";
        case WS_JSON_LOG_LEVEL_ERROR:         return "WS_JSON_ERROR";
        default:                        return "WS_JSON_UNKNOWN_LOG_LEVEL";
    };
}

void wsJsonSetLogLevel(int32_t level) {
    _wsJsonLogLevel = level;
}

wsJson* wsJsonInitObject(const char* key) {
    if (!key) {
        WS_JSON_LOG_API_DUMP("Object key is null");
    }

    wsJson* obj = WS_JSON_MALLOC(sizeof(wsJson));
    if (!obj) {
        WS_JSON_LOG_ERROR("Failed to allocate object: %s", key);
        return NULL;
    }
    memset(obj, 0, sizeof(wsJson));
    obj->object.children = NULL;
    obj->object.childCount = 0;
    obj->object.childCapacity = 0;
    obj->type = WS_JSON_OBJECT;
    if (key) strncpy(obj->key, key, sizeof(obj->key) - 1);
    return obj;
}

wsJson* wsJsonInitString(const char* key, const char* val) {
    wsJson* obj = WS_JSON_MALLOC(sizeof(wsJson));
    if (!obj) {
        WS_JSON_LOG_ERROR("Failed to allocate memory for json object: %s\n", key);
        return NULL;
    }
    memset(obj, 0, sizeof(wsJson));
    obj->type = WS_JSON_STRING;
    if (key) strncpy(obj->key, key, sizeof(obj->key) - 1);
    if (val) { 
        size_t length = strlen(val);
        obj->stringValue = WS_JSON_MALLOC(length + 1);
        strncpy(obj->stringValue, val, length);
        obj->stringValue[length] = '\0';
    }
    return obj;
}

wsJson* wsJsonInitNumber(const char* key, double val) {
    wsJson* obj = WS_JSON_MALLOC(sizeof(wsJson));
    if (!obj) {
        WS_JSON_LOG_ERROR("Failed to allocate memory for json object: %s\n", key);
        return NULL;
    }
    memset(obj, 0, sizeof(wsJson));
    obj->type = WS_JSON_NUMBER;
    if (key) strncpy(obj->key, key, sizeof(obj->key) - 1);
    obj->numberValue = val;
    return obj;
}

wsJson* wsJsonInitBool(const char* key, bool val) {
    wsJson* obj = WS_JSON_MALLOC(sizeof(wsJson));
    if (!obj) {
        WS_JSON_LOG_ERROR("Failed to allocate memory for json object: %s\n", key);
        return NULL;
    }
    memset(obj, 0, sizeof(wsJson));
    obj->type = WS_JSON_BOOL;
    if (key) strncpy(obj->key, key, sizeof(obj->key) - 1);
    obj->boolValue = val;
    return obj;
}

wsJson* wsJsonInitArray(const char* key) {
    wsJson* obj = WS_JSON_MALLOC(sizeof(wsJson));
    if (!obj) {
        WS_JSON_LOG_ERROR("Failed to allocate memory for json array: %s\n", key);
        return NULL;
    }
    memset(obj, 0, sizeof(wsJson));
    obj->array.elements = NULL;
    obj->array.elementCount = 0;
    obj->array.elementCapacity = 0;
    obj->type = WS_JSON_ARRAY;
    if (key) strncpy(obj->key, key, sizeof(obj->key) - 1);
    return obj;
}

wsJson* wsJsonInitNull(const char* key) {
    wsJson* obj = WS_JSON_MALLOC(sizeof(wsJson));
    if (!obj) {
        WS_JSON_LOG_ERROR("Failed to allocate memory for json null: %s\n", key);
        return NULL;
    }
    memset(obj, 0, sizeof(wsJson));
    obj->type = WS_JSON_NULL;
    if (key) strncpy(obj->key, key, sizeof(obj->key) - 1);
    return obj;
}

void wsJsonAddField(wsJson *parent, wsJson *child) {
    if (!parent || parent->type != WS_JSON_OBJECT || !child) return;

    if (parent->object.childCount >= parent->object.childCapacity) {
        int32_t newCap = parent->object.childCapacity == 0 ? 4 : parent->object.childCapacity * 2;
        parent->object.children = WS_JSON_REALLOC(parent->object.children, sizeof(wsJson) * newCap);
        parent->object.childCapacity = newCap;
    }
    parent->object.children[parent->object.childCount++] = child;
}

void wsJsonAddElement(wsJson *array, wsJson *element) {
    if (!array || array->type != WS_JSON_ARRAY || !element) return;
    
    if (array->array.elementCount >= array->array.elementCapacity) {
        int32_t newCap = array->array.elementCapacity == 0 ? 4 : array->array.elementCapacity * 2;
        array->array.elements = WS_JSON_REALLOC(array->array.elements, sizeof(wsJson) * newCap);
        array->array.elementCapacity = newCap;
    }
    array->array.elements[array->array.elementCount++] = element;
}

int32_t wsJsonToString(wsJson *obj, char *out, size_t size) {
    if (!obj) {
        WS_JSON_LOG_ERROR("Input json obj is NULL\n");
        return WS_ERROR;
    }
    if (!out) {
        WS_JSON_LOG_ERROR("Input buffer for output is NULL\n");
        return WS_ERROR;
    }
    
    size_t used = 0;

    switch (obj->type) {
        case WS_JSON_STRING:
            used += snprintf(out + used, size - used, "\"%s\"", obj->stringValue);
            break;
        case WS_JSON_NUMBER:
            used += snprintf(out + used, size - used, "%g", obj->numberValue);
            break;
        case WS_JSON_BOOL:
            used += snprintf(out + used, size - used, "%s", obj->boolValue ? "true" : "false");
            break;
        case WS_JSON_NULL:
            used += snprintf(out + used, size - used, "null");
            break;
        case WS_JSON_OBJECT:
            used += snprintf(out + used, size - used, "{");
            for (int32_t i = 0; i < obj->object.childCount; i++) {
                wsJson* child = obj->object.children[i];
                if (i > 0) {
                    used += snprintf(out + used, size - used, ",");
                }
                used += snprintf(out + used, size - used, "\"%s\": ", child->key);
                wsJsonToString(child, out + used, size - used);
                used = strlen(out);
            }
            used += snprintf(out + used, size - used, "}");
            break;
        case WS_JSON_ARRAY:
            used += snprintf(out + used, size - used, "[");
            for (int32_t i = 0; i < obj->array.elementCount; i++) {
                wsJson* element = obj->array.elements[i];
                if (i > 0) {
                    used += snprintf(out + used, size - used, ",");
                }
                wsJsonToString(element, out + used, size - used);
                used = strlen(out);
            }
            used += snprintf(out + used, size - used, "]");
            break;
        default:
            WS_JSON_LOG_ERROR("Failed to parse json into string\n");
            return WS_ERROR;
    }

    return used;
}

size_t addIndent(char* out, size_t size, int32_t indent) {
    size_t used = 0;
    for (int32_t i = 0; i < indent; i++) {
        if (used < size - 1) out[used++] = ' ';
    }
    return used;
}

int32_t wsJsonToStringPrettyInternal(wsJson *obj, char *out, size_t size, int32_t indent) {
    if (!obj) {
        WS_JSON_LOG_ERROR("Input json obj is NULL\n");
        return WS_ERROR;
    }
    if (!out) {
        WS_JSON_LOG_ERROR("Input buffer for output is NULL\n");
        return WS_ERROR;
    }
    
    size_t used = 0;

    switch (obj->type) {
        case WS_JSON_STRING:
            used += snprintf(out + used, size - used, "\"%s\"", obj->stringValue);
            break;
        case WS_JSON_NUMBER:
            used += snprintf(out + used, size - used, "%g", obj->numberValue);
            break;
        case WS_JSON_BOOL:
            used += snprintf(out + used, size - used, "%s", obj->boolValue ? "true" : "false");
            break;
        case WS_JSON_NULL:
            used += snprintf(out + used, size - used, "null");
            break;
        case WS_JSON_OBJECT:
            used += snprintf(out + used, size - used, "{\n");
            for (int32_t i = 0; i < obj->object.childCount; i++) {
                wsJson* child = obj->object.children[i];
                if (i > 0) {
                    used += snprintf(out + used, size - used, ",\n");
                }
                used += addIndent(out + used, size - used, indent + 4);
                used += snprintf(out + used, size - used, "\"%s\": ", child->key);
                int32_t written = wsJsonToStringPrettyInternal(child, out + used, size - used, indent + 4);
                if (written < 0) return WS_ERROR;
                used += written;
            }
            used += snprintf(out + used, size - used, "\n");
            used += addIndent(out + used, size - used, indent);
            used += snprintf(out + used, size - used, "}");
            break;
        case WS_JSON_ARRAY:
            used += snprintf(out + used, size - used, "[\n");
            for (int32_t i = 0; i < obj->array.elementCount; i++) {
                wsJson* element = obj->array.elements[i];
                if (i > 0) {
                    used += snprintf(out + used, size - used, ",\n");
                }
                used += addIndent(out + used, size - used, indent + 4);
                int32_t written = wsJsonToStringPrettyInternal(element, out + used, size - used, indent + 4);
                if (written < 0) return WS_ERROR;
                used += written;
            }
            used += snprintf(out + used, size - used, "\n");
            used += addIndent(out + used, size - used, indent);
            used += snprintf(out + used, size - used, "]");
            break;
        default:
            WS_JSON_LOG_ERROR("Failed to parse json into string\n");
            return WS_ERROR;
    }

    return used;
}

int32_t wsJsonToStringPretty(wsJson *obj, char *out, size_t size) {
    if (!obj || !out) {
        WS_JSON_LOG_ERROR("Invalid JSON or buffer\n");
        return WS_ERROR;
    }
    size_t used = wsJsonToStringPrettyInternal(obj, out, size, 0);
    out[used] = '\0';
    return (int32_t)used;
}

static char* skipWhitespaces(const char* string) {
    while (*string && isspace((unsigned char)*string)) string++;
    return (char*)string;
}

static char* parseString(const char** string) {
    (*string)++; // skip "
    const char* start = *string;

    // Find end (also handle escape sequences)
    while (**string) {
        if (**string == '\\') {
            (*string)++;
            if (**string) (*string)++;
        }
        else if (**string == '"') {
            break;
        }
        else {
            (*string)++;
        }
    }
    //while (**string && **string != '"') (*string)++;
    size_t len = *string - start;

    char* out = WS_JSON_MALLOC(len + 1);
    if (!out) {
        WS_JSON_LOG_ERROR("Failed to allocate for parsing json key\n");
        return NULL;
    }
    strncpy(out, start, len);
    out[len] = '\0';
    if (**string == '"') (*string)++; // skip closing " 
    return out;
}

static wsJson* parseValue(const char** string);

static wsJson* parseArray(const char** string) {
    wsJson* array = wsJsonInitArray(NULL);
    if (!array) {
        WS_JSON_LOG_ERROR("Failed to allocate json array\n");
        return NULL;
    }

    *string = skipWhitespaces(*string);
    if (**string != '[') {
        WS_JSON_LOG_ERROR("Failed to parse array: missing '['\n");
        wsJsonFree(array);
        return NULL;
    }
    (*string)++;

    while (**string) {
        *string = skipWhitespaces(*string);
        if (**string == ']') {
            (*string)++;
            break;
        }

        wsJson* element = parseValue(string);
        if (!element) {
            WS_JSON_LOG_ERROR("Failed to parse array element\n");
            wsJsonFree(array);
            return NULL;
        }

        wsJsonAddElement(array, element);

        *string = skipWhitespaces(*string);
        if (**string == ',') (*string)++;
    }

    return array;
}

static wsJson* parseValue(const char** string) {
    (*string) = skipWhitespaces(*string);

    // Is String 
    if (**string == '"') {
        char* val = parseString(string);
        if (!val) {
            WS_JSON_LOG_ERROR("Failed to parse json value when parsing string\n");
            return NULL;
        }
        wsJson* node = WS_JSON_CALLOC(1, sizeof(wsJson));
        if (!node) {
            WS_JSON_LOG_ERROR("Failed to allocate json node when parsing string\n");
            WS_JSON_FREE(val);
            return NULL;
        }
        node->type = WS_JSON_STRING;

        size_t valLen = strlen(val);
        node->stringValue = WS_JSON_MALLOC(valLen + 1);
        strncpy(node->stringValue, val, valLen);
        node->stringValue[valLen] = '\0';
        WS_JSON_FREE(val);
        return node;
    }
    
    // Is Field/Object 
    else if (**string == '{') {
        return wsStringToJson(string);
    }

    // Is Digit 
    else if (isdigit(**string) || **string == '-') {
        char* endPtr;
        double num = strtod(*string, &endPtr);
        wsJson* node = WS_JSON_CALLOC(1, sizeof(wsJson));
        if (!node) {
            WS_JSON_LOG_ERROR("Failed to allocate json node when parsing string\n");
            return NULL;
        }
        node->type = WS_JSON_NUMBER;
        node->numberValue = num;
        *string = endPtr;
        return node;
    }

    // Is Bool (true)
    else if (strncmp(*string, "true", 4) == 0) {
        wsJson* node = WS_JSON_CALLOC(1, sizeof(wsJson));
        if (!node) {
            WS_JSON_LOG_ERROR("Failed to allocate json node when parsing string\n");
            return NULL;
        }
        node->type = WS_JSON_BOOL;
        node->boolValue = true;
        *string += 4;
        return node;
    }

    // Is Bool (false)
    else if (strncmp(*string, "false", 5) == 0) {
        wsJson* node = WS_JSON_CALLOC(1, sizeof(wsJson));
        if (!node) {
            WS_JSON_LOG_ERROR("Failed to allocate json node when parsing string\n");
            return NULL;
        }
        node->type = WS_JSON_BOOL;
        node->boolValue = false;
        *string += 5;
        return node;
    }

    // Is Null
    else if (strncmp(*string, "null", 4) == 0) {
        wsJson* node = WS_JSON_CALLOC(1, sizeof(wsJson));
        if (!node) {
            WS_JSON_LOG_ERROR("Failed to allocate json node when parsing null\n");
            return NULL;
        }
        node->type = WS_JSON_NULL;
        *string += 4;
        return node;
    }

    // Is Array
    else if (**string == '[') {
        return parseArray(string);
    }

    return NULL;
}

wsJson* wsStringToJson(const char** string) {
    if (!string) {
        WS_JSON_LOG_ERROR("Invalid input paramerter is NULL\n");
        return NULL;
    }

    wsJson* root = wsJsonInitObject(NULL);
    if (!root) {
        WS_JSON_LOG_ERROR("Failed to allocate json object\n");
        return NULL;
    }

    *string = skipWhitespaces(*string);
    if (**string != '{') {
        WS_JSON_LOG_ERROR("Failed to convert string to json\n");
        wsJsonFree(root);
        return NULL;
    }
    (*string)++;

    while (**string) {
        *string = skipWhitespaces(*string);
        if (**string == '}') {
            (*string)++;
            break;
        }

        // read key
        char* key = parseString(string);
        if (!key) {
            WS_JSON_LOG_ERROR("Failed to parse json key\n");
            wsJsonFree(root);
            return NULL;
        }

        *string = skipWhitespaces(*string);
        if (**string != ':') {
            WS_JSON_FREE(key);
            return NULL;
        }
        (*string)++;

        // Read value
        *string = skipWhitespaces(*string);
        wsJson* val = parseValue(string);
        if (!val) {
            WS_JSON_LOG_ERROR("Failed to parse json value\n");
            WS_JSON_FREE(key);
            wsJsonFree(root);
            return NULL;
        }

        size_t keyLen = strlen(key);
        if (keyLen + 1 > WS_JSON_MAX_KEY_SIZE) {
            WS_JSON_LOG_ERROR("Json key Size is too long\n");
            WS_JSON_FREE(key);
            wsJsonFree(val);
            wsJsonFree(root);
            return NULL;
        }
        strncpy(val->key, key, keyLen);
        val->key[keyLen] = '\0';
        wsJsonAddField(root, val);

        *string = skipWhitespaces(*string);
        if (**string == ',') (*string)++;

        WS_JSON_FREE(key);
    }

    return root;
}

wsJson* wsJsonGetNonPath(wsJson* obj, const char* key) {
    if (!obj || !key) {
        WS_JSON_LOG_ERROR("Invalid input is NULL\n");
        return NULL;
    } 
    if (obj->type != WS_JSON_OBJECT) {
        WS_JSON_LOG_ERROR("Obj is not from type WS_JSON_OBJECT\n");
        return NULL;
    }

    for (int32_t i = 0; i < obj->object.childCount; i++) {
        wsJson* child = obj->object.children[i];
        if (strcmp(child->key, key) == 0) {
            return child;
        }
    }
    return NULL;
}

wsJson* wsJsonGet(wsJson* obj, const char* key) {
    if (!obj || !key) {
        WS_JSON_LOG_ERROR("Invalid input is NULL\n");
        return NULL;
    } 
    if (obj->type != WS_JSON_OBJECT) {
        WS_JSON_LOG_ERROR("Obj is not from type WS_JSON_OBJECT\n");
        return NULL;
    }

    const char* start = key;
    const char* dot;
    wsJson* current = obj;

    while (current && (dot = strchr(start, '.'))) {
        size_t len = (size_t)(dot - start);
        char path[WS_JSON_MAX_KEY_SIZE];
        if (len >= sizeof(path)) len = sizeof(path) - 1;
        memcpy(path, start, len);
        path[len] = '\0';

        current = wsJsonGetNonPath(current, path);
        start = dot + 1;
    }

    if (current && *start) {
        current = wsJsonGetNonPath(current, start);
    }

    return current;
}

char* wsJsonGetString(wsJson* obj, const char* key) {
    wsJson* child = wsJsonGet(obj, key);
    if (child && child->type == WS_JSON_STRING) {
        return child->stringValue;
    }
    return NULL;
}

int32_t wsJsonGetStringEx(wsJson *obj, const char *key, char *out, size_t size) {
    wsJson* child = wsJsonGet(obj, key);
    if (child && child->type == WS_JSON_STRING) {
        memcpy(out, child->stringValue, size);
        return WS_OK;
    }
    return WS_ERROR;
}

double wsJsonGetNumber(wsJson *obj, const char *key) {
    wsJson* child = wsJsonGet(obj, key);
    if (child && child->type == WS_JSON_NUMBER) {
        return child->numberValue;
    }
    return WS_ERROR;
}

bool wsJsonGetBool(wsJson* obj, const char* key) {
    wsJson* child = wsJsonGet(obj, key);
    if (child && child->type == WS_JSON_BOOL) {
        return child->boolValue;
    }
    return WS_ERROR;
}

int32_t wsJsonGetArrayLen(wsJson* obj, const char* key) {
    wsJson* child = wsJsonGet(obj, key);
    if (child && child->type == WS_JSON_ARRAY) {
        return child->array.elementCount;
    }
    return WS_ERROR;
}

wsJson* wsJsonGetArrayAt(wsJson* obj, const char* key, int32_t index) {
    wsJson* child = wsJsonGet(obj, key);
    if (child && child->type == WS_JSON_ARRAY) {
        if (index < 0 || index >= child->array.elementCount) return NULL;
        return child->array.elements[index];
    }
    return NULL;
}

int32_t wsJsonSetStringExplicit(wsJson *obj, const char *key, const char *val) {
    size_t length = strlen(val);

    wsJson* child = wsJsonGet(obj, key);
    if (child && child->type == WS_JSON_STRING) {
        if (child->stringValue) WS_JSON_FREE(child->stringValue);

        child->stringValue = WS_JSON_MALLOC(length + 1);
        strncpy(child->stringValue, val, length);
        child->stringValue[length] = '\0';
        return WS_OK;
    }
    return WS_ERROR;
}

int32_t wsJsonSetNumberExplicit(wsJson *obj, const char *key, double val) {
    wsJson* child = wsJsonGet(obj, key);
    if (child && child->type == WS_JSON_NUMBER) {
        child->numberValue = val;
        return WS_OK;
    }
    return WS_ERROR;
}

int32_t wsJsonSetBoolExplicit(wsJson *obj, const char *key, bool val) {
    wsJson* child = wsJsonGet(obj, key);
    if (child && child->type == WS_JSON_BOOL) {
        child->boolValue = val;
        return WS_OK;
    }
    return WS_ERROR;
}

int32_t wsJsonSetNullToObject(wsJson* obj, const char *key, wsJson *fields) {
    wsJson* child = wsJsonGet(obj, key);
    if (child && child->type == WS_JSON_NULL) {
        child->type = WS_JSON_OBJECT;

        child->object.children = fields->object.children;
        child->object.childCount = fields->object.childCount;
        child->object.childCapacity = fields->object.childCapacity;

        fields->object.children = NULL;
        fields->object.childCount = 0;
        fields->object.childCapacity = 0;
        WS_JSON_FREE(fields);
        return WS_OK;
    }
    return WS_ERROR;
}

int32_t wsJsonSetNullToString(wsJson *obj, const char *key, const char *val) {
    wsJson* child = wsJsonGet(obj, key);
    if (child && child->type == WS_JSON_NULL) {
        child->type = WS_JSON_STRING;
        size_t length = strlen(val);
        child->stringValue = WS_JSON_MALLOC(length + 1);
        strncpy(child->stringValue, val, length);
        child->stringValue[length] = '\0';
        return WS_OK;
    }
    return WS_ERROR;
}

int32_t wsJsonSetNullToNumber(wsJson *obj, const char *key, double val) {
    wsJson* child = wsJsonGet(obj, key);
    if (child && child->type == WS_JSON_NULL) {
        child->type = WS_JSON_NUMBER;
        child->numberValue = val;
        return WS_OK;
    }
    return WS_ERROR;
}

int32_t wsJsonSetNullToBool(wsJson *obj, const char *key, bool val) {
    wsJson* child = wsJsonGet(obj, key);
    if (child && child->type == WS_JSON_NULL) {
        child->type = WS_JSON_BOOL;
        child->boolValue = val;
        return WS_OK;
    }
    return WS_ERROR;
}

int32_t wsJsonSetNullToArray(wsJson *obj, const char *key, wsJson *array) {
    wsJson* child = wsJsonGet(obj, key);
    if (child && child->type == WS_JSON_NULL) {
        child->type = WS_JSON_ARRAY;

        child->array.elements = array->array.elements;
        child->array.elementCount = array->array.elementCount;
        child->array.elementCapacity = array->array.elementCapacity;

        array->array.elements = NULL;
        array->array.elementCount = 0;
        array->array.elementCapacity = 0;
        WS_JSON_FREE(array);
        return WS_OK;
    }
    return WS_ERROR;
}

int32_t wsJsonSetString(wsJson *obj, const char *key, const char *val) {
    wsJson* child = wsJsonGet(obj, key);
    if (child) {
        if (child->type == WS_JSON_STRING) return wsJsonSetStringExplicit(obj, key, val);
        else if (child->type == WS_JSON_NULL) return wsJsonSetNullToString(obj, key, val);
        return WS_ERROR;
    }
    return WS_ERROR;
}

int32_t wsJsonSetNumber(wsJson *obj, const char *key, double val) {
    wsJson* child = wsJsonGet(obj, key);
    if (child) {
        if (child->type == WS_JSON_NUMBER) return wsJsonSetNumberExplicit(obj, key, val);
        else if (child->type == WS_JSON_NULL) return wsJsonSetNullToNumber(obj, key, val);
        return WS_ERROR;
    }
    return WS_ERROR;
}

int32_t wsJsonSetBool(wsJson *obj, const char *key, bool val) {
    wsJson* child = wsJsonGet(obj, key);
    if (child) {
        if (child->type == WS_JSON_BOOL) return wsJsonSetBoolExplicit(obj, key, val);
        else if (child->type == WS_JSON_NULL) return wsJsonSetNullToBool(obj, key, val);
        return WS_ERROR;
    }
    return WS_ERROR;
}

int32_t wsJsonSetElement(wsJson *obj, const char *key, int32_t index, wsJson *element) {
    wsJson* child = wsJsonGet(obj, key);
    if (child && child->type == WS_JSON_ARRAY) {
        if (index < 0 || index >= child->array.elementCount) return WS_ERROR;
        child->array.elements[index] = element;
    }
    return WS_ERROR;
}

void wsJsonFree(wsJson *obj) {
    if (!obj) {
        WS_JSON_LOG_ERROR("JSON obj is NULL on free!\n");
        return;
    }
    if (obj->type == WS_JSON_OBJECT) {
        for (int32_t i = 0; i < obj->object.childCount; i++) {
            wsJsonFree(obj->object.children[i]);
        }
        WS_JSON_FREE(obj->object.children);
    } 
    else if (obj->type == WS_JSON_ARRAY) {
        for (int32_t i = 0; i < obj->array.elementCount; i++) {
            wsJsonFree(obj->array.elements[i]);
        }
        WS_JSON_FREE(obj->array.elements);
    }
    else if (obj->type == WS_JSON_STRING) {
        WS_JSON_FREE(obj->stringValue);
    }
    WS_JSON_FREE(obj);
}

#endif // WS_JSON_IMPLEMENTATION

#endif // WS_JSON_H

