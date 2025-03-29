#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "vk.h"

// bad text editor (bEditor)

#define STARTING_LINE_AMOUNT 20
#define MAX_COLUMNS 32

#define MAX_FILE_NAME_LENGTH 32
#define MAX_VISIBLE_LINES 30

#define KEYDOWN_MASK 0x1

#define NEW_LINE_ADDITIONAL_DELAY 30
#define PROMPT_SLEEP_TIME 30
#define LOOP_DELAY 50

#define LINES_SHOWN 20

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif

#define CLEAR_COMMAND "cls"

void printArray(char **arr, unsigned short lines, unsigned short columns, bool showWriteHead, unsigned short cLine, unsigned short cCol);
char getKeyPressCharScandinavian();
void shiftRow(char **arr, unsigned short row, unsigned short rowLen, short amount, unsigned short fromIndex);
void shiftPointerArray(char ***arr, unsigned short arrLen, unsigned short fromIndex, bool shiftL);
void saveToFile(char **arr, unsigned short arrLen, char *fileName);
void loadFromFile(char ***arr, unsigned short *arrLen, char *fileName);
void fileNamePrompt(char **fileName);
void resetActiveArray(char ***arr, unsigned short *arrLen);
unsigned short fileLineAmount(FILE *filePtr);

// ctrl+q is to exit
// ctrl+s is to save
// ctrl+o is to open
// ctrl+n is to create new document

// in this program you use alt+key instead of ctrl+alt+key


int main() {
    unsigned short totalLines = STARTING_LINE_AMOUNT;

    char **workingArray = (char **)calloc(totalLines, sizeof(char *));

    for (unsigned short i = 0; i < totalLines; ++i) {
        workingArray[i] = malloc(MAX_COLUMNS * sizeof(char));
        for (int j = 0; j < MAX_COLUMNS; ++j) {
            workingArray[i][j] = ' ';
        }
    }

    unsigned long loopNumber = 0;
    unsigned short cLine = 0;
    unsigned short cCol = 0;
    bool showWriteHead = false;
    char inputC;

    // This prevents a weird behaviour where the editor just reads a bunch of characters when starting up.
    for (int i = 0; i < 100; ++i) {
        getKeyPressCharScandinavian();
        Sleep(1);
    }

    while (true) {
        if ((GetKeyState(VK_CONTROL) >> 15)) {
            if (GetKeyState(VK_LETTER_Q) >> 15) {
                break;
            } else if (GetKeyState(VK_LETTER_S) >> 15) {
                GetAsyncKeyState(VK_LETTER_S);  // This is to prevent immediately getting "s" printed upon creating new document.
                char *fileName = (char *)calloc(MAX_FILE_NAME_LENGTH, sizeof(char));
                fileNamePrompt(&fileName);
                saveToFile(workingArray, totalLines, fileName);
                free(fileName);

            } else if (GetKeyState(VK_LETTER_O) >> 15) {
                printf("Are you sure you want to open a file? (y/n)");
                GetAsyncKeyState(VK_LETTER_O);
                Sleep(300);
                while (true) {
                    if (GetKeyState(VK_LETTER_Y) >> 15) {
                        GetAsyncKeyState(VK_LETTER_Y);
                        char *fileName = (char *)calloc(MAX_FILE_NAME_LENGTH, sizeof(char));
                        fileNamePrompt(&fileName);
                        loadFromFile(&workingArray, &totalLines, fileName);
                        cLine = 0;
                        cCol = 0;
                        free(fileName);
                        system(CLEAR_COMMAND);
                        printArray(workingArray, totalLines, MAX_COLUMNS, showWriteHead, cLine, cCol);
                        break;
                    }

                    if (GetKeyState(VK_LETTER_N) >> 15) {
                        GetAsyncKeyState(VK_LETTER_N);
                        break;
                    }
                    Sleep(PROMPT_SLEEP_TIME);
                }

            } else if (GetKeyState(VK_LETTER_N) >> 15) {
                printf("Are you sure you want to create a document? (y/n)");
                GetAsyncKeyState(VK_LETTER_N);  // This is to prevent immediately getting "n" printed upon creating new document.
                Sleep(300);
                while (true) {
                    if (GetKeyState(VK_LETTER_Y) >> 15) {
                        GetAsyncKeyState(VK_LETTER_Y);
                        cLine = 0;
                        cCol = 0;
                        resetActiveArray(&workingArray, &totalLines);
                        break;
                    }
                    if (GetKeyState(VK_LETTER_N) >> 15) {
                        GetAsyncKeyState(VK_LETTER_N);
                        break;
                    }
                    Sleep(PROMPT_SLEEP_TIME);
                }
            }
        }

        if (GetAsyncKeyState(VK_UP) & KEYDOWN_MASK) {
            system(CLEAR_COMMAND);
            showWriteHead = true;
            --cLine;
            if (cLine == USHRT_MAX) {
                cLine = 0;
            }
            printArray(workingArray, totalLines, MAX_COLUMNS, showWriteHead, cLine, cCol);
        } else if (GetAsyncKeyState(VK_DOWN) & KEYDOWN_MASK) {
            system(CLEAR_COMMAND);
            showWriteHead = true;
            ++cLine;
            if (cLine > totalLines - 1) {
                cLine = totalLines - 1;
            }
            printArray(workingArray, totalLines, MAX_COLUMNS, showWriteHead, cLine, cCol);
        } else if (GetAsyncKeyState(VK_LEFT) & KEYDOWN_MASK) {
            system(CLEAR_COMMAND);
            showWriteHead = true;
            --cCol;
            printArray(workingArray, totalLines, MAX_COLUMNS, showWriteHead, cLine, cCol);
        } else if (GetAsyncKeyState(VK_RIGHT) & KEYDOWN_MASK) {
            system(CLEAR_COMMAND);
            showWriteHead = true;
            ++cCol;
            printArray(workingArray, totalLines, MAX_COLUMNS, showWriteHead, cLine, cCol);
        } else if (GetKeyState(VK_BACK) >> 15) {
            if (cCol == 0 && cLine != 0) {
                char *oldLine = workingArray[cLine];
                shiftPointerArray(&workingArray, totalLines, cLine, true);
                char **nPtr = (char **)realloc(workingArray, (totalLines - 1) * sizeof(char *));
                if (nPtr != NULL) {
                    workingArray = nPtr;
                    free(oldLine);
                    --totalLines;
                    if (cLine != 0) {
                        --cLine;
                    }
                    cCol = 1;  // This tries to prevent you from accidentally deleting a bunch of lines in one slip up
                }
            } else {
                shiftRow(workingArray, cLine, MAX_COLUMNS, -1, cCol);
                workingArray[cLine][MAX_COLUMNS - 1] = ' ';
                --cCol;
            }
            if (totalLines == 0) {
                printf("NO LINES\n");
                break;
            }
            system(CLEAR_COMMAND);
            showWriteHead = true;
            printArray(workingArray, totalLines, MAX_COLUMNS, showWriteHead, cLine, cCol);
        } else if (GetKeyState(VK_RETURN) >> 15) {
            system(CLEAR_COMMAND);
            char *newLine = (char *)malloc(MAX_COLUMNS * sizeof(char));
            if (newLine != NULL) {
                char **newPtr = (char **)realloc(workingArray, (totalLines + 1) * sizeof(char *));
                if (newPtr != NULL) {
                    workingArray = newPtr;
                    ++cLine;
                    ++totalLines;
                    shiftPointerArray(&workingArray, totalLines, cLine, false);
                    for (unsigned short i = 0; i < MAX_COLUMNS; ++i) {
                        newLine[i] = ' ';
                    }
                    workingArray[cLine] = newLine;
                    printArray(workingArray, totalLines, MAX_COLUMNS, showWriteHead, cLine, cCol);
                    Sleep(NEW_LINE_ADDITIONAL_DELAY);
                } else {
                    free(newLine);
                }
            }
        } else {
            inputC = getKeyPressCharScandinavian();
            if (inputC != 0) {
                system(CLEAR_COMMAND);
                showWriteHead = true;
                shiftRow(workingArray, cLine, MAX_COLUMNS, 1, cCol);
                workingArray[cLine][cCol + 1] = inputC;
                ++cCol;
                printArray(workingArray, totalLines, MAX_COLUMNS, showWriteHead, cLine, cCol);
            }
        }

        if (loopNumber % 12 == 0) {
            system(CLEAR_COMMAND);
            showWriteHead ^= true;  // flip
            printArray(workingArray, totalLines, MAX_COLUMNS, showWriteHead, cLine, cCol);
        }

        if (cCol == USHRT_MAX) {
            cCol = 0;
        } else if (cCol > MAX_COLUMNS - 1) {
            cCol = MAX_COLUMNS - 1;
        }

        if (cLine == USHRT_MAX) {
            cLine = 0;
        } else if (cLine > totalLines - 1) {
            cLine = totalLines - 1;
        }

        ++loopNumber;
        Sleep(LOOP_DELAY);
    }

    for (unsigned short i = 0; i < totalLines; ++i) {
        free(workingArray[i]);
    }
    free(workingArray);

    printf("bEditor exited");

    return 0;
}

void printArray(char **arr, unsigned short lines, unsigned short columns, bool showWriteHead, unsigned short cLine, unsigned short cCol) {
    printf("================ bEditor ================\n");

    unsigned short startingLine;
    unsigned short endLine;

    if (lines < LINES_SHOWN) {
        startingLine = 0;
        endLine = lines;
    } else if (cLine + (LINES_SHOWN / 2) <= LINES_SHOWN) {
        startingLine = 0;
        endLine = LINES_SHOWN;
    } else if (cLine + (LINES_SHOWN / 2) >= lines) {
        startingLine = lines - LINES_SHOWN;
        endLine = lines;
    } else {
        startingLine = cLine - (LINES_SHOWN / 2);
        endLine = cLine + (LINES_SHOWN / 2);
    }

    for (unsigned short lineNumber = startingLine; lineNumber < endLine; ++lineNumber) {
        printf("%d: ", lineNumber + 1);
        if (lineNumber == cLine && showWriteHead) {
            for (unsigned short columnNumber = 0; columnNumber < columns; ++columnNumber) {
                if (columnNumber == cCol) {
                    printf("%c", arr[lineNumber][columnNumber]);
                    printf("|");
                } else {
                    printf("%c", arr[lineNumber][columnNumber]);
                }
            }
        } else {
            for (unsigned short columnNumber = 0; columnNumber < columns; ++columnNumber) {
                printf("%c", arr[lineNumber][columnNumber]);
            }
        }
        printf("\n");
    }
}

void shiftRow(char **arr, unsigned short row, unsigned short rowLen, short amount, unsigned short fromIndex) {
    if (amount == 0) {
        return;
    }

    if (amount > 0) {
        for (unsigned short shifts = 0; shifts < amount; ++shifts) {
            for (unsigned short i = rowLen; i > fromIndex; --i) {
                arr[row][i] = arr[row][i - 1];
            }
        }

    } else if (amount < 0) {
        for (short shifts = 0; shifts > amount; --shifts) {
            for (unsigned short i = fromIndex; i < rowLen; ++i) {
                arr[row][i] = arr[row][i + 1];
            }
        }
    }
}

void shiftPointerArray(char ***arr, unsigned short arrLen, unsigned short fromIndex, bool shiftL) {
    if (shiftL) {
        for (unsigned short i = fromIndex; i < arrLen; ++i) {
            (*arr)[i] = (*arr)[i + 1];
        }
        (*arr)[arrLen - 1] = NULL;

    } else {
        for (unsigned short i = arrLen - 1; i > fromIndex; --i) {
            (*arr)[i] = (*arr)[i - 1];
        }
        (*arr)[fromIndex] = NULL;
    }
}


char getKeyPressCharScandinavian() {
    bool shiftIsDown = GetKeyState(VK_LSHIFT) >> 15;
    bool altIsDown = GetKeyState(VK_MENU) >> 15;

    if (shiftIsDown) {
        for (unsigned char vKey = VK_LETTER_A; vKey < VK_LETTER_Z + 1; ++vKey) {
            if (GetAsyncKeyState(vKey) & KEYDOWN_MASK) {
                return vKey;
            }
        }
    } else {
        for (unsigned char vKey = VK_LETTER_A; vKey < VK_LETTER_Z + 1; ++vKey) {
            if (GetAsyncKeyState(vKey) & KEYDOWN_MASK) {
                return vKey + 32;
            }
        }
    }

    if (GetAsyncKeyState(VK_NUMBER_0) & KEYDOWN_MASK) {
        if (shiftIsDown) {
            return '=';
        } else if (altIsDown) {
            return '}';
        } else {
            return '0';
        }
    }

    if (GetAsyncKeyState(VK_NUMBER_1) & KEYDOWN_MASK) {
        if (shiftIsDown) {
            return '!';
        } else {
            return '+';
        }
    }

    if (GetAsyncKeyState(VK_NUMBER_2) & KEYDOWN_MASK) {
        if (shiftIsDown) {
            return '"';
        } else if (altIsDown) {
            return '@';
        } else {
            return '2';
        }
    }

    if (GetAsyncKeyState(VK_NUMBER_3) & KEYDOWN_MASK) {
        if (shiftIsDown) {
            return '#';
        } else {
            return '3';
        }
    }

    if (GetAsyncKeyState(VK_NUMBER_4) & KEYDOWN_MASK) {
        if (altIsDown) {
            return '$';
        } else {
            return '4';
        }
    }

    if (GetAsyncKeyState(VK_NUMBER_5) & KEYDOWN_MASK) {
        if (shiftIsDown) {
            return '%';
        } else {
            return '5';
        }
    }

    if (GetAsyncKeyState(VK_NUMBER_6) & KEYDOWN_MASK) {
        if (shiftIsDown) {
            return '&';
        } else {
            return '6';
        }
    }

    if (GetAsyncKeyState(VK_NUMBER_7) & KEYDOWN_MASK) {
        if (shiftIsDown) {
            return '/';
        } else if (altIsDown) {
            return '{';
        } else {
            return '7';
        }
    }

    if (GetAsyncKeyState(VK_NUMBER_8) & KEYDOWN_MASK) {
        if (shiftIsDown) {
            return '(';
        } else if (altIsDown) {
            return '[';
        } else {
            return '8';
        }
    }

    if (GetAsyncKeyState(VK_NUMBER_9) & KEYDOWN_MASK) {
        if (shiftIsDown) {
            return ')';
        } else if (altIsDown) {
            return ']';
        } else {
            return '9';
        }
    }

    if (GetAsyncKeyState(VK_SPACE) & KEYDOWN_MASK) {
        return ' ';
    }

    if (GetAsyncKeyState(VK_OEM_PERIOD) & KEYDOWN_MASK) {
        if (shiftIsDown) {
            return ':';
        } else {
            return '.';
        }
    }

    if (GetAsyncKeyState(VK_OEM_COMMA) & KEYDOWN_MASK) {
        if (shiftIsDown) {
            return ';';
        } else {
            return ',';
        }
    }

    if (GetAsyncKeyState(VK_OEM_MINUS) & KEYDOWN_MASK) {
        if (shiftIsDown) {
            return '_';
        } else {
            return '-';
        }
    }

    if (GetAsyncKeyState(VK_OEM_PLUS) & KEYDOWN_MASK) {
        if (shiftIsDown) {
            return '?';
        } else if (altIsDown) {
            return '\\';
        } else {
            return '+';
        }
    }

    if (GetAsyncKeyState(VK_OEM_102) & KEYDOWN_MASK) {
        if (shiftIsDown) {
            return '>';
        } else {
            return '<';
        }
    }

    if (GetAsyncKeyState(VK_OEM_2) & KEYDOWN_MASK) {
        if (shiftIsDown) {
            return '*';
        } else {
            return '\'';
        }
    }

    if (GetAsyncKeyState(VK_OEM_1) & KEYDOWN_MASK) {
        if (shiftIsDown) {
            return '^';
        } else if (altIsDown) {
            return '~';
        }
    }

    if ((GetAsyncKeyState(VK_OEM_4) & KEYDOWN_MASK) && shiftIsDown) {
        return '`';
    }

    return 0;
}

void saveToFile(char **arr, unsigned short arrLen, char *fileName) {
    FILE *filePtr = fopen(fileName, "w");
    if (filePtr == NULL) {
        return;
    }
    fclose(filePtr);
    filePtr = fopen(fileName, "a");
    if (filePtr == NULL) {
        return;
    }

    unsigned short lineLength = MAX_COLUMNS;

    for (unsigned short i = 0; i < arrLen; ++i) {
        for (unsigned short j = MAX_COLUMNS - 1; j > -1; --j) {
            if (arr[i][j] != ' ') {
                lineLength = j + 1;
                break;
            }
        }

        for (unsigned short j = 0; j < lineLength; ++j) {
            fwrite(&(arr[i][j]), sizeof(char), 1, filePtr);
        }
        fwrite("\n", sizeof(char), 1, filePtr);
    }

    fclose(filePtr);
}

void loadFromFile(char ***arr, unsigned short *arrLen, char *fileName) {
    FILE *filePtr = fopen(fileName, "r");
    if (filePtr == NULL) {
        return;
    }

    unsigned short currentLine = 0;
    unsigned short cCol = 0;
    char c;

    unsigned short lines = fileLineAmount(filePtr);

    char **newArr = (char **)calloc(lines, sizeof(char *));

    for (unsigned short i = 0; i < lines; ++i) {
        newArr[i] = (char *)malloc(MAX_COLUMNS * sizeof(char));
        if (newArr[i] == NULL) {
            for (unsigned short j = 0; j < i; ++j) {
                free(newArr[j]);
            }
            free(newArr);
            fclose(filePtr);
            return;
        }
        for (int j = 0; j < MAX_COLUMNS; ++j) {
            newArr[i][j] = ' ';
        }
    }

    while (true) {
        if (feof(filePtr)) {
            break;
        }
        c = fgetc(filePtr);
        if (c == '\n') {
            ++currentLine;
            cCol = 0;
            continue;
        }
        newArr[currentLine][cCol] = c;
        ++cCol;
    }

    for (unsigned short i = 0; i < *arrLen; ++i) {
        free((*arr)[i]);
    }

    *arr = newArr;
    *arrLen = currentLine;
}

unsigned short fileLineAmount(FILE *filePtr) {
    char c;
    unsigned short lineAmount = 1;
    while (!feof(filePtr)) {
        c = fgetc(filePtr);
        if (c == '\n') {
            ++lineAmount;
        }
    }
    fseek(filePtr, 0, SEEK_SET);
    return lineAmount;
}

void resetActiveArray(char ***arr, unsigned short *arrLen) {
    if (*arrLen > STARTING_LINE_AMOUNT) {
        for (unsigned short i = STARTING_LINE_AMOUNT; i < *arrLen; ++i) {
            free((*arr)[i]);
        }
    }
    char **newPtr = (char **)realloc(*arr, STARTING_LINE_AMOUNT * sizeof(char *));
    if (newPtr == NULL) {
        return;
    }
    *arrLen = STARTING_LINE_AMOUNT;

    *arr = newPtr;
    for (unsigned short i = 0; i < *arrLen; ++i) {
        if ((*arr)[i] == NULL) {
            (*arr)[i] = (char *)malloc(MAX_COLUMNS * sizeof(char));
        }
        for (int j = 0; j < MAX_COLUMNS; ++j) {
            (*arr)[i][j] = ' ';
        }
    }
}

void fileNamePrompt(char **fileName) {
    char inputC;
    Sleep(300);
    system(CLEAR_COMMAND);
    printf("Enter file name: ");
    unsigned char fnCol = 0;
    while (true) {
        inputC = getKeyPressCharScandinavian();

        if (GetKeyState(VK_RETURN) >> 15) {
            break;
        }

        if (GetKeyState(VK_BACK) >> 15) {
            if (fnCol != 0) {
                --fnCol;
            }
            (*fileName)[fnCol] = '\0';
            system(CLEAR_COMMAND);
            printf("Enter file name: ");
            for (unsigned char i = 0; i < MAX_FILE_NAME_LENGTH; ++i) {
                printf("%c", (*fileName)[i]);
            }
        }

        if (inputC != 0) {
            (*fileName)[fnCol] = inputC;
            ++fnCol;
            system(CLEAR_COMMAND);
            printf("Enter file name: ");
            for (unsigned char i = 0; i < MAX_FILE_NAME_LENGTH; ++i) {
                printf("%c", (*fileName)[i]);
            }
            Sleep(PROMPT_SLEEP_TIME);
        }

        Sleep(PROMPT_SLEEP_TIME);
    }
}
