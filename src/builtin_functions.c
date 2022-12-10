#include "builtin_functions.h"
#include "visitor.h"
#include "AST.h"
#include "lib/io.h"
#include "thread.h"
#include "lib/http.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

AST_T* try_run_builtin_function(visitor_T* visitor, AST_T* node)
{
    AST_T** args = node->function_call_arguments;
    size_t args_size = node->function_call_arguments_size;
    
    if (strcmp(node->function_call_name, "print") == 0)
    {
        for (size_t i = 0; i < args_size; i++)
        {
            AST_T* visited_ast = visitor_visit(visitor, args[i]);
            switch (visited_ast->type)
            {
                case AST_STRING: printf("%s", visited_ast->string_value); break;
                case AST_INT: printf("%i", visited_ast->ast_int); break;
                case AST_DOUBLE: printf("%f", visited_ast->ast_double); break;
                case AST_BOOLEAN:
                    switch (visited_ast->boolean_value)
                    {
                        case 1: printf("true"); break;
                        case 0: printf("false"); break;
                    }
                    break;
                case AST_NOOP: printf("null"); break;
                default: break;
            }
        }
        return init_ast(AST_NOOP);
    }
    if (strcmp(node->function_call_name, "println") == 0)
    {
        for (size_t i = 0; i < args_size; i++)
        {
            AST_T* visited_ast = visitor_visit(visitor, args[i]);
            switch (visited_ast->type)
            {
                case AST_STRING: printf("%s", visited_ast->string_value); break;
                case AST_INT: printf("%i", visited_ast->ast_int); break;
                case AST_DOUBLE: printf("%f", visited_ast->ast_double); break;
                case AST_BOOLEAN:
                    switch (visited_ast->boolean_value)
                    {
                        case 1: printf("true"); break;
                        case 0: printf("false"); break;
                    }
                    break;
                case AST_NOOP: printf("null"); break;
                default: break;
            }
        }
        printf("\n");
        return init_ast(AST_NOOP);
    }

    if (strcmp(node->function_call_name, "exit") == 0)
    {
        if (args_size != 1)
        {
            printf("\nruntime error:\n    function 'exit()' takes 1 argument\n");
            exit(1);
        }

        AST_T* visited_ast = visitor_visit(visitor, args[0]);
        exit(visited_ast->ast_int);
    }

    if (strcmp(node->function_call_name, "system") == 0)
    {
        if (args_size != 1)
        {
            printf("\nruntime error:\n    function 'system()' takes 1 argument\n");
            exit(1);
        }
        AST_T* visited_ast = visitor_visit(visitor, args[0]);
        AST_T* ast = init_ast(AST_INT);
        ast->ast_int = system(visited_ast->string_value);
        return ast;
    }

    if (strcmp(node->function_call_name, "input") == 0)
    {
        AST_T* ast = init_ast(AST_STRING);
        for (size_t i = 0; i < args_size; i++)
        {
            AST_T* visited_ast = visitor_visit(visitor, args[i]);
            switch (visited_ast->type)
            {
                case AST_STRING: printf("%s", visited_ast->string_value); break;
                case AST_INT: printf("%i", visited_ast->ast_int); break;
                case AST_DOUBLE: printf("%f", visited_ast->ast_double); break;
                case AST_BOOLEAN:
                    switch (visited_ast->boolean_value)
                    {
                        case 1: printf("true"); break;
                        case 0: printf("false"); break;
                    }
                    break;
                case AST_NOOP: printf("(null)"); break;
                default: break;
            }
        }
        char value[16384];
        fgets(value, sizeof(value), stdin);
        ast->string_value = malloc(sizeof(value));
        strcpy(ast->string_value, value);
        ast->string_value[strlen(ast->string_value) - 1] = '\0';
        return ast;
    }

    if (strcmp(node->function_call_name, "atoi") == 0)
    {
        if (args_size != 1)
        {
            printf("\nruntime error:\n    function 'atoi()' takes 1 argument\n");
            exit(1);
        }
        AST_T* visited_ast = init_ast(AST_INT);
        AST_T* ast = visitor_visit(visitor, args[0]);
        switch (ast->type)
        {
            case AST_STRING: visited_ast->ast_int = atoi(ast->string_value); break;
            case AST_DOUBLE: visited_ast->ast_int = (int)ast->ast_double; break;
            case AST_BOOLEAN: visited_ast->ast_int = (int)ast->boolean_value; break;
            case AST_INT: visited_ast->ast_int = ast->ast_int; break;
            default: visited_ast->ast_int = 0; break;
        }
        return visited_ast;
    }
    if (strcmp(node->function_call_name, "atod") == 0)
    {
        if (args_size != 1)
        {
            printf("\nruntime error:\n    function 'atod()' takes 1 argument\n");
            exit(1);
        }
        AST_T* visited_ast = init_ast(AST_INT);
        AST_T* ast = visitor_visit(visitor, args[0]);
        switch (ast->type)
        {
            case AST_STRING: visited_ast->ast_double = atof(ast->string_value); break;
            case AST_DOUBLE: visited_ast->ast_double = ast->ast_double; break;
            case AST_BOOLEAN: visited_ast->ast_double = (double)ast->boolean_value; break;
            case AST_INT: visited_ast->ast_double = ast->ast_int; break;
            default: visited_ast->ast_double = 0; break;
        }
        return visited_ast;
    }
    if (strcmp(node->function_call_name, "atob") == 0)
    {
        if (args_size != 1)
        {
            printf("\nruntime error:\n    function 'atob()' takes 1 argument\n");
            exit(1);
        }
        AST_T* visited_ast = init_ast(AST_BOOLEAN);
        visited_ast->boolean_value = atoi(visitor_visit(visitor, args[0])->string_value);
        return visited_ast;
    }
    if (strcmp(node->function_call_name, "free") == 0)
    {
        scope_remove_variable_definition(
            node->scope,
            args[0]->variable_name
        );
        scope_remove_variable_definition(
            visitor->scope,
            args[0]->variable_name
        );
        return node;
    }
    if (strcmp(node->function_call_name, "async") == 0)
    {
        async_exec(visitor_visit(visitor, args[0]));
        return init_ast(AST_NOOP);
    }
    if (strcmp(node->function_call_name, "sleep") == 0)
    {
        if (args_size != 1)
        {
            printf("\nruntime error:\n    function 'sleep()' takes 1 argument\n");
            exit(1);
        }
        AST_T* visited_ast = visitor_visit(visitor, args[0]);
        usleep(visited_ast->ast_double * 1000);
        usleep(visited_ast->ast_int * 1000);
        return visited_ast;
    }
    if (strcmp(node->function_call_name, "http") == 0)
    {
        AST_T* header = visitor_visit(visitor, args[0]);
        AST_T* ast = init_ast(AST_STRING);
        ast->string_value = http_request(header->string_value);
        return ast;
    }
    if (strcmp(node->function_call_name, "strcat") == 0)
    {
        if (args_size != 2)
        {
            printf("\nruntime error:\n    function 'strcat()' takes 2 arguments\n");
            exit(1);
        }
        AST_T* a = visitor_visit(visitor, args[0]);
        AST_T* b = visitor_visit(visitor, args[1]);
        char* res = malloc(sizeof(a->string_value)+sizeof(b->string_value)+1);
        sprintf(res, "%s%s", a->string_value, b->string_value);
        AST_T* ast = init_ast(AST_STRING);
        ast->string_value = res;
        return ast;
    }
    if (strcmp(node->function_call_name, "typeof") == 0)
    {
        if (args_size != 1)
        {
            printf("\nruntime error:\n    function 'typeof()' takes 1 argument\n");
            exit(1);
        }
        AST_T* inp = visitor_visit(visitor, args[0]);
        AST_T* res = init_ast(AST_STRING);
        switch (inp->type)
        {
            case AST_STRING: res->string_value = "string"; break;
            case AST_INT: res->string_value = "int"; break;
            case AST_DOUBLE: res->string_value = "double"; break;
            case AST_LONG: res->string_value = "long"; break;
            case AST_ARRAY: res->string_value = malloc(100); sprintf(res->string_value, "array[%li]", inp->array_size); break;
            case AST_CHAR: res->string_value = "char"; break;
            case AST_STREAM: res->string_value = "stream"; break;
            default: res->string_value = "void"; break;
        }
        return res;
    }
    if (strcmp(node->function_call_name, "open") == 0)
    {
        if (args_size != 1)
        {
            printf("\nruntime error:\n    function 'open()' takes 1 argument\n");
            exit(1);
        }
        AST_T* ast = init_ast(AST_STREAM);

        ast->stream = fopen(visitor_visit(visitor, args[0])->string_value, "wrb");

        return ast;
    }
    if (strcmp(node->function_call_name, "close") == 0)
    {
        if (args_size != 1)
        {
            printf("\nruntime error:\n    function 'close()' takes 1 argument\n");
            exit(1);
        }
        FILE* stream = visitor_visit(visitor, args[0])->stream;

        fclose(stream);

        return init_ast(AST_NOOP);
    }
    if (strcmp(node->function_call_name, "write") == 0)
    {
        if (args_size != 2)
        {
            printf("\nruntime error:\n    function 'write()' takes 2 argument\n");
            exit(1);
        }
        AST_T* stream = visitor_visit(visitor, args[0]);
        AST_T* value = visitor_visit(visitor, args[1]);
        
        fprintf(stream->stream, value->string_value);

        return init_ast(AST_NOOP);
    }
    if (strcmp(node->function_call_name, "read") == 0)
    {
        if (args_size != 1)
        {
            printf("\nruntime error:\n    function 'read()' takes 1 argument\n");
            exit(1);
        }
        FILE* stream = visitor_visit(visitor, args[0])->stream;
        
        char* buffer = 0;
        size_t length;

        if (stream)
        {
            fseek(stream, 0, SEEK_END);
            length = ftell(stream);
            fseek(stream, 0, SEEK_SET);

            buffer = calloc(length, length);

            if (buffer)
                fread(buffer, 1, length, stream);

            AST_T* ast = init_ast(AST_STRING);
            ast->string_value = buffer;

            return ast;
        }

        return init_ast(AST_NOOP);
    }
    if (strcmp(node->function_call_name, "remove") == 0)
    {
        char* file = visitor_visit(visitor, args[0])->string_value;
        AST_T* res = init_ast(AST_INT);
        res->ast_int = remove(file);
        return res;
    }
    
    return (void*) 0;
}