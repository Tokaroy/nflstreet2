/* T3DB query lexer: splits the text of a database query into tokens (operators, numbers,
   keywords and identifiers, bracketed strings) for the query compiler. */

#include "engine/t3db.h"

#define TDB_LEX_TOKEN(TYPE, ADVANCE, LENGTH)         \
    pLexer->mTokens[pLexer->mCount].mType = (TYPE); \
    p += (ADVANCE);                                 \
    pLexer->mTokens[pLexer->mCount++].mSrc = p - (LENGTH)

/* FAKEMATCH: each keyword jumps to the shared return instead of returning, only for the
   target's single return tail */
#define TDB_LEX_KEYWORD_GOTO(TYPE, LENGTH)                    \
    pLexer->mTokens[pLexer->mCount].mType = (TYPE);          \
    pLexer->mTokens[pLexer->mCount++].mSrc = p - (LENGTH);   \
    goto done

char *fn_80200DD4(TDbLexer_801FCE10 *pLexer, char *p);
char *fn_80201030(TDbLexer_801FCE10 *pLexer, char *p);

void fn_802009B0(TDbLexer_801FCE10 *pLexer, unsigned int start)
{
    char *p = pLexer->mSrc;

    pLexer->mTokens[1].mType = 0x49;
    pLexer->mTokens[1].mSrc = p;
    pLexer->mCount = start;
    while (p != 0 && pLexer->mCount <= 1) {
        if (*p == ' ') {
            p++;
        } else if ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') || *p == '_') {
            p = fn_80201030(pLexer, p);
        } else if (*p < 0) {
            TDB_LEX_TOKEN((unsigned char)*p - 0x80, 1, 1);
        } else if ((unsigned char)(*p - '0') <= 9 || (*p == '-' && (unsigned char)(p[1] - '0') <= 9)) {
            p = fn_80200DD4(pLexer, p);
        } else if (*p == '\'') {
            /* four-character constant between quotes */
            pLexer->mTokens[pLexer->mCount].mType = 0x33;
            pLexer->mTokens[pLexer->mCount].mValue.mPtr = (void *)4;
            p += 5;
            pLexer->mTokens[pLexer->mCount++].mSrc = p - 4;
            p++;
        } else if (*p == '%') {
            TDB_LEX_TOKEN(0x43, 1, 1);
        } else if (*p == '\0') {
            break;
        } else if (*p == '[') {
            p = fn_80201C70(pLexer, p);
        } else if (*p == '\n') {
            p++;
        } else if (*p == '\t') {
            p++;
        } else if (*p == '\r') {
            p++;
        } else if (*p == '\f') {
            p++;
        } else if (*p == '>') {
            if (*++p == '=') {
                TDB_LEX_TOKEN(0x35, 1, 2);
            } else {
                TDB_LEX_TOKEN(0x3B, 0, 1);
            }
        } else if (*p == '<') {
            if (*++p == '=') {
                TDB_LEX_TOKEN(0x34, 1, 2);
            } else {
                TDB_LEX_TOKEN(0x3A, 0, 1);
            }
        } else if (*p == '*') {
            TDB_LEX_TOKEN(0x41, 1, 1);
        } else if (*p == '&') {
            if (*++p == '&') {
                TDB_LEX_TOKEN(0x38, 1, 2);
            } else {
                TDB_LEX_TOKEN(0x46, 0, 1);
            }
        } else if (*p == '^') {
            TDB_LEX_TOKEN(0x45, 1, 1);
        } else if (*p == '|') {
            if (*++p == '|') {
                TDB_LEX_TOKEN(0x39, 1, 2);
            } else {
                TDB_LEX_TOKEN(0x47, 0, 1);
            }
        } else if (*p == '=') {
            if (*++p == '=') {
                TDB_LEX_TOKEN(0x36, 1, 2);
            } else {
                TDB_LEX_TOKEN(0x36, 0, 1);
            }
        } else if (*p == '!') {
            if (*++p == '=') {
                TDB_LEX_TOKEN(0x37, 1, 2);
            } else {
                TDB_LEX_TOKEN(0x3F, 0, 1);
            }
        } else if (*p == '(') {
            TDB_LEX_TOKEN(0x3C, 1, 1);
        } else if (*p == ')') {
            TDB_LEX_TOKEN(0x3D, 1, 1);
        } else if (*p == '.') {
            TDB_LEX_TOKEN(0x3E, 1, 1);
        } else if (*p == '+') {
            TDB_LEX_TOKEN(0x44, 1, 1);
        } else if (*p == '-') {
            TDB_LEX_TOKEN(0x40, 1, 1);
        } else if (*p == '~') {
            TDB_LEX_TOKEN(0x48, 1, 1);
        } else {
            TDB_LEX_TOKEN(-1, 1, 1);
        }
    }
    if (pLexer->mCount == 0) {
        TDB_LEX_TOKEN(0x49, 0, 0);
    }
    pLexer->mSrc = p;
}

char *fn_80200DD4(TDbLexer_801FCE10 *pLexer, char *p)
{
    int negative = 0;
    int value = 0;
    int scale;
    double real;
    char c;

    if (p[0] == '0' && (p[1] == 'X' || p[1] == 'x')) {
        p++;
        scale = 1;
        c = *++p;
        while ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')) {
            value *= scale;
            scale = 16;
            if (c >= '0' && c <= '9') {
                value += c - '0';
            } else if (c >= 'A' && c <= 'F') {
                value += c - 'A' + 10;
            } else if (c >= 'a' && c <= 'f') {
                value += c - 'a' + 10;
            }
            c = *++p;
        }
    } else {
        value = 0;
        scale = 1;
        if (*p == '-') {
            negative = 1;
            p++;
        }
        c = *p;
        while (c >= '0' && c <= '9') {
            value *= scale;
            value += c - '0';
            scale = 10;
            c = *++p;
        }
        if (*p == '.') {
            real = value;
            scale = 10;
            c = *++p;
            while (c >= '0' && c <= '9') {
                real += (double)(c - '0') / scale;
                scale *= 10;
                c = *++p;
            }
            if (c == 'f') {
                p++;
            }
            if (negative) {
                real = -real;
            }
            pLexer->mTokens[pLexer->mCount].mType = 0x31;
            pLexer->mTokens[pLexer->mCount].mValue.mFloat = real;
            pLexer->mTokens[pLexer->mCount++].mSrc = p;
            return p;
        }
        if (negative) {
            value = -value;
        }
    }
    pLexer->mTokens[pLexer->mCount].mType = 0x30;
    pLexer->mTokens[pLexer->mCount].mValue.mPtr = (void *)value;
    pLexer->mTokens[pLexer->mCount++].mSrc = p;
    return p;
}

char *fn_80201030(TDbLexer_801FCE10 *pLexer, char *p)
{
    char name[48];
    int length = 0;
    char c = *p;

    do {
        name[length++] = c;
        c = *++p;
    } while ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9') || *p == '_');

    switch (length) {
    case 2:
        if (name[0] == 'o' && name[1] == 'r') {
            TDB_LEX_KEYWORD_GOTO(0x39, 2);
        } else if (name[0] == 'o' && name[1] == 'n') {
            TDB_LEX_KEYWORD_GOTO(0x1D, 2);
        } else if (name[0] == 'b' && name[1] == 'y') {
            TDB_LEX_KEYWORD_GOTO(0x21, 2);
        }
        break;
    case 3:
        if (name[0] == 'a' && name[1] == 's' && name[2] == 'c') {
            TDB_LEX_KEYWORD_GOTO(0x22, 3);
        } else if (name[0] == 's' && name[1] == 'e' && name[2] == 't') {
            TDB_LEX_KEYWORD_GOTO(0x1E, 3);
        } else if (name[0] == 'a' && name[1] == 'n' && name[2] == 'd') {
            TDB_LEX_KEYWORD_GOTO(0x38, 3);
        } else if (name[0] == 'u' && name[1] == 's' && name[2] == 'e') {
            TDB_LEX_KEYWORD_GOTO(0x1F, 3);
        } else if (name[0] == 'f' && name[1] == 'o' && name[2] == 'r') {
            TDB_LEX_KEYWORD_GOTO(0x29, 3);
        } else if (name[0] == 'm' && name[1] == 'i' && name[2] == 'n') {
            TDB_LEX_KEYWORD_GOTO(0x2C, 3);
        } else if (name[0] == 'm' && name[1] == 'a' && name[2] == 'x') {
            TDB_LEX_KEYWORD_GOTO(0x2D, 3);
        } else if (name[0] == 's' && name[1] == 'u' && name[2] == 'm') {
            TDB_LEX_KEYWORD_GOTO(0x2E, 3);
        }
        break;
    case 4:
        if (name[0] == 'f' && name[1] == 'r' && name[2] == 'o' && name[3] == 'm') {
            TDB_LEX_KEYWORD_GOTO(0x16, 4);
        } else if (name[0] == 'i' && name[1] == 'n' && name[2] == 't' && name[3] == 'o') {
            TDB_LEX_KEYWORD_GOTO(0x17, 4);
        } else if (name[0] == 'd' && name[1] == 'e' && name[2] == 's' && name[3] == 'c') {
            TDB_LEX_KEYWORD_GOTO(0x23, 4);
        } else if (name[0] == 'j' && name[1] == 'o' && name[2] == 'i' && name[3] == 'n') {
            TDB_LEX_KEYWORD_GOTO(0x19, 4);
        } else if (name[0] == 'l' && name[1] == 'e' && name[2] == 'f' && name[3] == 't') {
            TDB_LEX_KEYWORD_GOTO(0x1A, 4);
        }
        break;
    case 5:
        if (name[0] == 'w' && name[1] == 'h' && name[2] == 'e' && name[3] == 'r' && name[4] == 'e') {
            TDB_LEX_KEYWORD_GOTO(0x14, 5);
        } else if (name[0] == 'f' && name[1] == 'e' && name[2] == 't' && name[3] == 'c' && name[4] == 'h') {
            TDB_LEX_KEYWORD_GOTO(0x13, 5);
        } else if (name[0] == 'o' && name[1] == 'r' && name[2] == 'd' && name[3] == 'e' && name[4] == 'r') {
            TDB_LEX_KEYWORD_GOTO(0x20, 5);
        } else if (name[0] == 'i' && name[1] == 'n' && name[2] == 'd' && name[3] == 'e' && name[4] == 'x') {
            TDB_LEX_KEYWORD_GOTO(0x26, 5);
        } else if (name[0] == 'i' && name[1] == 'n' && name[2] == 'n' && name[3] == 'e' && name[4] == 'r') {
            TDB_LEX_KEYWORD_GOTO(0x18, 5);
        } else if (name[0] == 'c' && name[1] == 'o' && name[2] == 'u' && name[3] == 'n' && name[4] == 't') {
            TDB_LEX_KEYWORD_GOTO(0x2B, 5);
        } else if (name[0] == 'o' && name[1] == 'u' && name[2] == 't' && name[3] == 'e' && name[4] == 'r') {
            TDB_LEX_KEYWORD_GOTO(0x1B, 5);
        }
        break;
    case 6:
        if (name[0] == 's' && name[1] == 'e' && name[2] == 'l' && name[3] == 'e' && name[4] == 'c' && name[5] == 't') {
            TDB_LEX_KEYWORD_GOTO(0x11, 6);
        } else if (name[0] == 'i' && name[1] == 'n' && name[2] == 's' && name[3] == 'e' && name[4] == 'r' && name[5] == 't') {
            TDB_LEX_KEYWORD_GOTO(0x12, 6);
        } else if (name[0] == 'u' && name[1] == 'p' && name[2] == 'd' && name[3] == 'a' && name[4] == 't' && name[5] == 'e') {
            TDB_LEX_KEYWORD_GOTO(0x1C, 6);
        } else if (name[0] == 'd' && name[1] == 'e' && name[2] == 'l' && name[3] == 'e' && name[4] == 't' && name[5] == 'e') {
            TDB_LEX_KEYWORD_GOTO(0x15, 6);
        } else if (name[0] == 'c' && name[1] == 'u' && name[2] == 'r' && name[3] == 's' && name[4] == 'o' && name[5] == 'r') {
            TDB_LEX_KEYWORD_GOTO(0x27, 6);
        } else if (name[0] == 'c' && name[1] == 'r' && name[2] == 'e' && name[3] == 'a' && name[4] == 't' && name[5] == 'e') {
            TDB_LEX_KEYWORD_GOTO(0x25, 6);
        } else if (name[0] == 'u' && name[1] == 'n' && name[2] == 'i' && name[3] == 'q' && name[4] == 'u' && name[5] == 'e') {
            TDB_LEX_KEYWORD_GOTO(0x2A, 6);
        }
        break;
    case 7:
        if (name[0] == 'd' && name[1] == 'e' && name[2] == 'c' && name[3] == 'l' && name[4] == 'a' && name[5] == 'r' && name[6] == 'e') {
            TDB_LEX_KEYWORD_GOTO(0x24, 7);
        }
        break;
    case 10:
        if (name[0] == 'f' && name[1] == 'a' && name[2] == 's' && name[3] == 't' && name[4] == 'c' && name[5] == 'u' && name[6] == 'r' && name[7] == 's' && name[8] == 'o' && name[9] == 'r') {
            TDB_LEX_KEYWORD_GOTO(0x28, 10);
        }
        break;
    }
    pLexer->mTokens[pLexer->mCount].mType = 0x2F;
    pLexer->mTokens[pLexer->mCount].mValue.mPtr = (void *)length;
    pLexer->mTokens[pLexer->mCount++].mSrc = p - length;
done:
    return p;
}

char *fn_80201C70(TDbLexer_801FCE10 *pLexer, char *p)
{
    unsigned int used = pLexer->mBufUsed;
    char c;

    do {
        c = *++p;
        pLexer->mBuffer[used++] = *p;
        if (c == '\\') {
            pLexer->mBuffer[used++] = *++p;
            c = *++p;
            pLexer->mBuffer[used++] = *p;
        }
    } while (c != ']' && c != '\0');
    used--;
    pLexer->mBuffer[used++] = '\0';
    if (c == '\0') {
        pLexer->mTokens[pLexer->mCount].mType = -1;
        pLexer->mTokens[pLexer->mCount++].mSrc = p;
        return p;
    }
    while (used & 3) {
        pLexer->mBuffer[used++] = '\0';
    }
    pLexer->mTokens[pLexer->mCount].mType = 0x32;
    pLexer->mTokens[pLexer->mCount].mValue.mPtr = &pLexer->mBuffer[pLexer->mBufUsed];
    pLexer->mTokens[pLexer->mCount++].mSrc = p + (-used - pLexer->mBufUsed);
    pLexer->mBufUsed = used;
    return p + 1;
}
