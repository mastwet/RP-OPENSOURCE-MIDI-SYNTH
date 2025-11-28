#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

#define MAXVARS 100
#define GLOBAL_BUFFER_ROWS 9
#define GLOBAL_BUFFER_COLS 64

class Calculator {
public:
    Calculator() {
        init_constants();
        clear_global_buffer();
        add_to_global_buffer(">>> ");
    }

    void process(const char* input) {
        TokenStream ts;
        error_flag = 0;
        line_buffer[0] = '\0';

        snprintf(line_buffer, sizeof(line_buffer), "%s\n", input);
        append_to_last_line(line_buffer);

        tokenize(input, &ts);
        if (!error_flag) {
            float result = comparison(&ts);
            if (!error_flag) {
                if (result == (int)result) {
                    snprintf(line_buffer, sizeof(line_buffer), "%d\n", (int)result);
                } else {
                    snprintf(line_buffer, sizeof(line_buffer), "%f\n", result);
                }
            }
        } else {
            snprintf(line_buffer, sizeof(line_buffer), "ERROR: TOKENIZATION FAILED.\n");
        }

        // 将输出结果添加到全局缓冲区
        add_to_global_buffer(line_buffer);

        // 计算完成后再次打印提示符，不换行
        add_to_global_buffer(">>> ");
    }

    char* get_global_buffer(uint8_t row) {
        return global_buffer[row];
    }

private:
    typedef enum {
        TOKEN_NUMBER, TOKEN_PLUS, TOKEN_MINUS, TOKEN_MULT, TOKEN_DIV, TOKEN_INTDIV, TOKEN_EXP,
        TOKEN_EQ, TOKEN_NEQ, TOKEN_GE, TOKEN_LE, TOKEN_GT, TOKEN_LT,
        TOKEN_LPAREN, TOKEN_RPAREN, TOKEN_FUNC, TOKEN_VAR, TOKEN_ASSIGN, TOKEN_END, TOKEN_ERROR
    } TokenType;

    struct Token {
        TokenType type;
        float value;
        char name[32];
        int position;
    };

    struct TokenStream {
        Token tokens[100];
        int pos;
    };

    struct Variable {
        char name[32];
        float value;
    };

    Variable vars[MAXVARS];
    int varCount = 0;
    int error_flag = 0;

    // 全局字符缓冲区
    char global_buffer[GLOBAL_BUFFER_ROWS][GLOBAL_BUFFER_COLS];
    int global_buffer_index = 0;  // 当前缓冲区行索引
    char line_buffer[GLOBAL_BUFFER_COLS];  // 单行缓冲区

    const char* protected_constants[3] = { "PI", "E", "PHI" };

    void clear_global_buffer() {
        for (int i = 0; i < GLOBAL_BUFFER_ROWS; i++) {
            global_buffer[i][0] = '\0';
        }
    }

    // 将新行内容添加到全局缓冲区
    void add_to_global_buffer(const char* line) {
        if (global_buffer_index >= GLOBAL_BUFFER_ROWS) {
            // 滚动删除最早的行
            for (int i = 1; i < GLOBAL_BUFFER_ROWS; i++) {
                strncpy(global_buffer[i - 1], global_buffer[i], GLOBAL_BUFFER_COLS - 1);
            }
            global_buffer_index = GLOBAL_BUFFER_ROWS - 1;
        }
        strncpy(global_buffer[global_buffer_index], line, GLOBAL_BUFFER_COLS - 1);
        global_buffer[global_buffer_index][GLOBAL_BUFFER_COLS - 1] = '\0';  // 确保行以 '\0' 结尾
        global_buffer_index++;
    }

    // 追加到上一行（不换行）
    void append_to_last_line(const char* line) {
        int current_len = strlen(global_buffer[global_buffer_index - 1]);
        int line_len = strlen(line);

        // 确保不会超过一行的最大限制
        if (current_len + line_len < GLOBAL_BUFFER_COLS) {
            strncat(global_buffer[global_buffer_index - 1], line, line_len);
        }
    }

    void init_constants() {
        setVar("PI", M_PI);
        setVar("E", 2.718281828459045f);
        setVar("PHI", 1.618033988749895f);
    }

    float function(TokenStream* ts, const char* funcName) {
        match(ts, TOKEN_FUNC);
        match(ts, TOKEN_LPAREN);
        float arg = expr(ts);
        match(ts, TOKEN_RPAREN);

        if (strcmp(funcName, "SIN") == 0) {
            return sinf(arg);
        } else if (strcmp(funcName, "COS") == 0) {
            return cosf(arg);
        } else if (strcmp(funcName, "TAN") == 0) {
            return tanf(arg);
        } else if (strcmp(funcName, "ASIN") == 0) {
            return asinf(arg);
        } else if (strcmp(funcName, "ACOS") == 0) {
            return acosf(arg);
        } else if (strcmp(funcName, "ATAN") == 0) {
            return atanf(arg);
        } else if (strcmp(funcName, "SQRT") == 0) {
            return sqrtf(arg);
        } else if (strcmp(funcName, "LOG") == 0) {
            return logf(arg);
        } else if (strcmp(funcName, "LOG10") == 0) {
            return log10f(arg);
        } else if (strcmp(funcName, "EXP") == 0) {
            return expf(arg);
        } else if (strcmp(funcName, "FABS") == 0) {
            return fabsf(arg);
        } else if (strcmp(funcName, "CEIL") == 0) {
            return ceilf(arg);
        } else if (strcmp(funcName, "FLOOR") == 0) {
            return floorf(arg);
        } else if (strcmp(funcName, "ROUND") == 0) {
            return roundf(arg);
        } else if (strcmp(funcName, "DEG2RAD") == 0) {
            return arg * (M_PI / 180.0f);
        } else if (strcmp(funcName, "RAD2DEG") == 0) {
            return arg * (180.0f / M_PI);
        } else {
            char error_msg[80];
            snprintf(error_msg, sizeof(error_msg), "UNKNOWN FUNCTION '%s'\n", funcName);
            add_to_global_buffer(error_msg);
            return 0;
        }
    }

    void tokenize(const char* input, TokenStream* ts) {
        int i = 0;
        int pos = 0;
        while (input[i] != '\0') {
            if (isdigit(input[i])) {
                ts->tokens[pos].type = TOKEN_NUMBER;
                ts->tokens[pos].value = strtof(&input[i], NULL);
                ts->tokens[pos].position = i;
                while (isdigit(input[i]) || input[i] == '.') i++;
            } else if (input[i] == '=' && input[i + 1] == '=') {
                ts->tokens[pos].type = TOKEN_EQ;
                ts->tokens[pos].position = i;
                i += 2;
            } else if (input[i] == '!' && input[i + 1] == '=') {
                ts->tokens[pos].type = TOKEN_NEQ;
                ts->tokens[pos].position = i;
                i += 2;
            } else if (input[i] == '>' && input[i + 1] == '=') {
                ts->tokens[pos].type = TOKEN_GE;
                ts->tokens[pos].position = i;
                i += 2;
            } else if (input[i] == '<' && input[i + 1] == '=') {
                ts->tokens[pos].type = TOKEN_LE;
                ts->tokens[pos].position = i;
                i += 2;
            } else if (input[i] == '>') {
                ts->tokens[pos].type = TOKEN_GT;
                ts->tokens[pos].position = i;
                i++;
            } else if (input[i] == '<') {
                ts->tokens[pos].type = TOKEN_LT;
                ts->tokens[pos].position = i;
                i++;
            } else if (input[i] == '+' && input[i + 1] != '+') {
                ts->tokens[pos].type = TOKEN_PLUS;
                ts->tokens[pos].position = i;
                i++;
            } else if (input[i] == '-' && input[i + 1] != '-') {
                ts->tokens[pos].type = TOKEN_MINUS;
                ts->tokens[pos].position = i;
                i++;
            } else if (input[i] == '*' && input[i + 1] != '*') {
                ts->tokens[pos].type = TOKEN_MULT;
                ts->tokens[pos].position = i;
                i++;
            } else if (input[i] == '/' && input[i + 1] != '/') {
                ts->tokens[pos].type = TOKEN_DIV;
                ts->tokens[pos].position = i;
                i++;
            } else if (input[i] == '/' && input[i + 1] == '/') {
                ts->tokens[pos].type = TOKEN_INTDIV;
                ts->tokens[pos].position = i;
                i += 2;
            } else if (input[i] == '*' && input[i + 1] == '*') {
                ts->tokens[pos].type = TOKEN_EXP;
                ts->tokens[pos].position = i;
                i += 2;
            } else if (input[i] == '(') {
                ts->tokens[pos].type = TOKEN_LPAREN;
                ts->tokens[pos].position = i;
                i++;
            } else if (input[i] == ')') {
                ts->tokens[pos].type = TOKEN_RPAREN;
                ts->tokens[pos].position = i;
                i++;
            } else if (isalpha(input[i])) {
                int start = i;
                while (isalnum(input[i])) i++;
                strncpy(ts->tokens[pos].name, &input[start], i - start);
                ts->tokens[pos].name[i - start] = '\0';
                ts->tokens[pos].position = start;
                if (input[i] == '(') {
                    ts->tokens[pos].type = TOKEN_FUNC;
                } else {
                    ts->tokens[pos].type = TOKEN_VAR;
                }
            } else if (input[i] == '=') {
                ts->tokens[pos].type = TOKEN_ASSIGN;
                ts->tokens[pos].position = i;
                i++;
            } else if (isspace(input[i])) {
                i++;
                continue;
            } else {
                char error_msg[80];
                snprintf(error_msg, sizeof(error_msg), "UNKNOWN CHARACTER '%c' AT POSITION %d\n", input[i], i);
                add_to_global_buffer(error_msg);
                ts->tokens[pos].type = TOKEN_ERROR;
                ts->tokens[pos].position = i;
                i++;
            }
            pos++;
        }
        ts->tokens[pos].type = TOKEN_END;
        ts->tokens[pos].position = i;
        ts->pos = 0;
    }

    void syntax_error(const char* reason, const char* token, int position) {
        char error_msg[80];
        snprintf(error_msg, sizeof(error_msg), "ERROR: %s AT %s | %d\n", reason, token, position);
        add_to_global_buffer(error_msg);
        error_flag = 1;
    }

    void match(TokenStream* ts, TokenType expected) {
        if (ts->tokens[ts->pos].type == expected) {
            ts->pos++;
        } else {
            syntax_error("UNEXPECTED TOKEN", ts->tokens[ts->pos].name, ts->tokens[ts->pos].position);
        }
    }

    float findVar(const char* name) {
        for (int i = 0; i < varCount; i++) {
            if (strcmp(vars[i].name, name) == 0) {
                return vars[i].value;
            }
        }
        return NAN;
    }

    void setVar(const char* name, float value) {
        for (int i = 0; i < varCount; i++) {
            if (strcmp(vars[i].name, name) == 0) {
                vars[i].value = value;
                return;
            }
        }
        strcpy(vars[varCount].name, name);
        vars[varCount].value = value;
        varCount++;
    }

    int is_protected_constant(const char* name) {
        for (int i = 0; i < sizeof(protected_constants) / sizeof(protected_constants[0]); i++) {
            if (strcmp(name, protected_constants[i]) == 0) {
                return 1;
            }
        }
        return 0;
    }

    float parseVar(TokenStream* ts) {
        char varName[32];
        strcpy(varName, ts->tokens[ts->pos].name);
        match(ts, TOKEN_VAR);
        if (ts->tokens[ts->pos].type == TOKEN_ASSIGN) {
            if (is_protected_constant(varName)) {
                syntax_error("ATTEMPT TO MODIFY A PROTECTED CONSTANT", varName, ts->tokens[ts->pos].position);
                return NAN;
            }
            match(ts, TOKEN_ASSIGN);
            float val = expr(ts);
            setVar(varName, val);
            return val;
        } else {
            float val = findVar(varName);
            if (isnan(val)) {
                syntax_error("UNDEFINED VARIABLE", varName, ts->tokens[ts->pos].position);
            }
            return val;
        }
    }

    float factor(TokenStream* ts) {
        Token token = ts->tokens[ts->pos];
        if (token.type == TOKEN_NUMBER) {
            match(ts, TOKEN_NUMBER);
            return token.value;
        } else if (token.type == TOKEN_VAR) {
            return parseVar(ts);
        } else if (token.type == TOKEN_FUNC) {
            return function(ts, token.name);
        } else if (token.type == TOKEN_LPAREN) {
            match(ts, TOKEN_LPAREN);
            float result = expr(ts);
            match(ts, TOKEN_RPAREN);
            return result;
        } else {
            syntax_error("UNEXPECTED FACTOR", token.name, token.position);
            return 0;
        }
    }

    float exponentiation(TokenStream* ts) {
        float base = factor(ts);
        if (ts->tokens[ts->pos].type == TOKEN_EXP) {
            match(ts, TOKEN_EXP);
            float exponent = factor(ts);
            return powf(base, exponent);
        }
        return base;
    }

    float term(TokenStream* ts) {
        float result = exponentiation(ts);
        while (ts->tokens[ts->pos].type == TOKEN_MULT || ts->tokens[ts->pos].type == TOKEN_DIV || 
               ts->tokens[ts->pos].type == TOKEN_INTDIV) {
            Token token = ts->tokens[ts->pos];
            if (token.type == TOKEN_MULT) {
                match(ts, TOKEN_MULT);
                result *= exponentiation(ts);
            } else if (token.type == TOKEN_DIV) {
                match(ts, TOKEN_DIV);
                result /= exponentiation(ts);
            } else if (token.type == TOKEN_INTDIV) {
                match(ts, TOKEN_INTDIV);
                result = (int)result / (int)exponentiation(ts);
            }
        }
        return result;
    }

    float expr(TokenStream* ts) {
        float result = term(ts);
        while (ts->tokens[ts->pos].type == TOKEN_PLUS || ts->tokens[ts->pos].type == TOKEN_MINUS) {
            Token token = ts->tokens[ts->pos];
            if (token.type == TOKEN_PLUS) {
                match(ts, TOKEN_PLUS);
                result += term(ts);
            } else if (token.type == TOKEN_MINUS) {
                match(ts, TOKEN_MINUS);
                result -= term(ts);
            }
        }
        return result;
    }

    float comparison(TokenStream* ts) {
        float result = expr(ts);
        while (ts->tokens[ts->pos].type == TOKEN_EQ || ts->tokens[ts->pos].type == TOKEN_NEQ ||
               ts->tokens[ts->pos].type == TOKEN_GE || ts->tokens[ts->pos].type == TOKEN_LE ||
               ts->tokens[ts->pos].type == TOKEN_GT || ts->tokens[ts->pos].type == TOKEN_LT) {
            Token token = ts->tokens[ts->pos];
            if (token.type == TOKEN_EQ) {
                match(ts, TOKEN_EQ);
                result = (result == expr(ts)) ? 1 : 0;
            } else if (token.type == TOKEN_NEQ) {
                match(ts, TOKEN_NEQ);
                result = (result != expr(ts)) ? 1 : 0;
            } else if (token.type == TOKEN_GE) {
                match(ts, TOKEN_GE);
                result = (result >= expr(ts)) ? 1 : 0;
            } else if (token.type == TOKEN_LE) {
                match(ts, TOKEN_LE);
                result = (result <= expr(ts)) ? 1 : 0;
            } else if (token.type == TOKEN_GT) {
                match(ts, TOKEN_GT);
                result = (result > expr(ts)) ? 1 : 0;
            } else if (token.type == TOKEN_LT) {
                match(ts, TOKEN_LT);
                result = (result < expr(ts)) ? 1 : 0;
            }
        }
        return result;
    }
};