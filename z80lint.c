#include <stdio.h>
#include <strings.h>
#include <ctype.h>
#include <stdlib.h>

struct operand {
    char workBuffer[256];
    char *operand1, *operand2;
    char *operand1Offset, *operand2Offset;
    int firstParameterIsPointer;
    int secondParameterIsPointer;
};

struct asmLine {
    char lineWorkBuffer[256];
    char originalLine[256];
    struct operand op;
    char *label;
    char *opcode;
    char *operand;
    char *comment;
};

int isnull(char *str)
{
    if (str == NULL || str[0] == '\0') {
        return 1;
    }
    
    return 0;
}

int isEmpty(struct asmLine theLine) {
    if (isnull(theLine.comment) && isnull(theLine.label) && isnull(theLine.opcode) && isnull(theLine.operand)) {
        return 1;
    }
    
    return 0;
}

int isComment(struct asmLine theLine) {
    if (!isnull(theLine.comment) && isnull(theLine.label) && isnull(theLine.opcode) && isnull(theLine.operand)) {
        return 1;
    }
    
    return 0;
}

int isCode(struct asmLine theLine) {
    if (!isnull(theLine.opcode)) {
        return 1;
    }
    
    return 0;
}

int hasZ80Operand(char *opcode) {
    int ret = 1;
    
    if (strcasecmp(opcode, "nop") == 0 ||
        strcasecmp(opcode, "di") == 0
    ) {
        ret = 0;
    }
    
    return ret;
}

int has6809Operand(char *opcode) {
    int ret = 1;

    if (strcasecmp(opcode, "nop") == 0 ||
        strcasecmp(opcode, "clra") == 0 ||
        strcasecmp(opcode, "clrb") == 0 ||
        strcasecmp(opcode, "inca") == 0 ||
        strcasecmp(opcode, "incb") == 0 ||
        strcasecmp(opcode, "sync") == 0 ||
        strcasecmp(opcode, "abx") == 0
    ) {
        ret = 0;
        }

        return ret;
}

int hasOperand(char *opcode) {
    return (hasZ80Operand(opcode) || has6809Operand(opcode));
}

void prettyPrint(struct asmLine theLine) {
    char prettyLine[256];
    
    if (isComment(theLine)) {
        strcpy(prettyLine, theLine.comment);
    } else {
        sprintf(prettyLine, "%-15s   %-8s   %-12s   %s", theLine.label, theLine.opcode, theLine.operand, theLine.comment);
    }
    printf("%s\n", prettyLine);
}

void identifyLine(struct asmLine *theLine) {
    if (theLine->label != NULL) {
        printf("label \"%s\", ", theLine->label);
    }
    if (theLine->opcode != NULL) {
        printf("opcode \"%s\", ", theLine->opcode);
    }
    if (theLine->operand != NULL) {
        printf("operand \"%s\", ", theLine->operand);
    }
    if (theLine->comment != NULL) {
        printf("commment \"%s\", ", theLine->comment);
    }
    printf("\n");
}

void parseLine(char *sourceLine, struct asmLine *theLine) {
    theLine->label = "";
    theLine->opcode = "";
    theLine->operand = "";
    theLine->comment = "";
    
    strncpy(theLine->lineWorkBuffer, sourceLine, 255);
    char *line = theLine->lineWorkBuffer;
    strcpy(theLine->originalLine, theLine->lineWorkBuffer);

    if (line[0] == '*' || line[0] == ';')
    {
        // the entire line is a comment
        theLine->comment = theLine->lineWorkBuffer;
    } else {
        // does the line have a leading space or tab?
        if (line[0] == ' ' || line[0] == '\t') {
            // if so, there is no label... eat leading spaces/tabs
            while (isspace(*line)) {
                line++;
            }
            theLine->opcode = strtok(theLine->lineWorkBuffer, " \t");
            
            // determine if the opcode has an operand
            if (hasOperand(theLine->opcode)) {
                theLine->operand = strtok(NULL, " \t");
                if (theLine->operand == NULL) theLine->operand = "";
            }
            theLine->comment = strtok(NULL, "");
            if (theLine->comment == NULL) theLine->comment = "";
        } else {
            theLine->label = strtok(line, " \t");
            if (theLine->label == NULL) theLine->label = "";
            theLine->opcode = strtok(NULL, " \t");
            if (theLine->opcode == NULL) theLine->opcode = "";
            // determine if the opcode has an operand
            if (hasOperand(theLine->opcode)) {
                theLine->operand = strtok(NULL, " \t");
                if (theLine->operand == NULL) theLine->operand = "";
            }
            theLine->comment = strtok(NULL, "");
            if (theLine->comment == NULL) theLine->comment = "";
        }
    }
}

void splitOperand(struct asmLine theLine)
{
    strcpy(theLine.op.workBuffer, theLine.operand);
    theLine.op.operand1 = strtok(theLine.operand, ",");
    if (theLine.op.operand1 == NULL) theLine.op.operand1 = "";
    theLine.op.operand2 = strtok(NULL, "");
    if (theLine.op.operand2 == NULL) theLine.op.operand2 = "";
}

void change(char *var)
{
    if (strcasecmp(var, "HL") == 0) {
        strcpy(var, "X");
    }
    else if (strcasecmp(var, "SP") == 0) {
        strcpy(var, "S");
    }
    else if (strcasecmp(var, "IX") == 0) {
        strcpy(var, "X");
    }
    else if (strcasecmp(var, "DE") == 0) {
        strcpy(var, "Y");
    }
    else if (strcasecmp(var, "AF") == 0) {
        strcpy(var, "A,CC");
    }
    else if (strcasecmp(var, "BC") == 0) {
        strcpy(var, "U");
    }
    else if (strcasecmp(var, "H") == 0) {
        strcpy(var, "A");
    }
    else if (strcasecmp(var, "B") == 0) {
        strcpy(var, "B");
    }
    else if (strcasecmp(var, "L") == 0) {
        strcpy(var, "B");
    }
    else if (strcasecmp(var, "A") == 0) {
        strcpy(var, "A");
    }
    else strcpy(var, "??");
}

void handleZ80Operand(struct asmLine z80Line, int *isPointer, char **z80Register, int *offset) {
    splitOperand(z80Line);
    if (z80Line.op.operand1[0] == '(') {
        // first parameter is pointer
        z80Line.op.firstParameterIsPointer = 1;
    }
    if (z80Line.op.operand2[0] == '(') {
        // second parameter is pointer
        z80Line.op.secondParameterIsPointer = 1;
    }

    // handle offset (e.g. (ix+$00))
}

void handleLD(struct asmLine z80Line, struct asmLine *m6809Line) {
    char first[128], second[128];
    
    splitOperand(z80Line);
    strcpy(first, z80Line.op.operand1);
    strcpy(second, z80Line.op.operand2);
    if ((first[0] != '(') && (second[0] != '(') && ((first[0] != '$' && !isdigit(first[0]) ) && (second[0] != '$' && !isdigit(second[0]) ))) {
        change(first);
        change(second);
        sprintf(m6809Line->lineWorkBuffer, "%s tfr %s,%s ; Z80: %s", z80Line.label, second, first, z80Line.originalLine);
    } else {
        char opcode[256], operand[256], temp[256];
        int point, offset;
        char *reg;
        handleZ80Operand(z80Line, &point, &reg, &offset);
        
        strcpy(opcode, "LD");
        
        if (second[0] == '$' || isdigit(second[0])) {
            strcpy(temp,"#");
            strcat(temp,second);
            strcpy(second,temp);
            point = 0;
        }
        if (first[0] == '(') {
            point = 1;
            strncpy(temp, first+1, strlen(first)-2);
            temp[strlen(first)-2] = '\0';
            //reverse order of first, second
            strcpy(first,second);
            strcpy(second,temp);
            strcpy(opcode, "ST");
        }
        if (second[0] == '(') {
            point = 1;
            strncpy(temp, second+1, strlen(second)-2);
            temp[strlen(second)-2] = '\0';
            strcpy(second, temp);
        }
        if (point == 1) {
            strcpy(temp, "");
            if (strlen(second) < 3) {
                strcat(temp,",");
            }
            change(second);
            strcat(temp, second);
            strcat(temp, "");
            strcpy(second, temp);
        }
        change(first);
        strcat(opcode,first);
        strcpy(operand,second);
        if ((strncmp(opcode, "ST#$00     ,X" ,13) == 0)) {
            strcpy(opcode, "CLR");
            strcpy(operand, ",X");
        }
        if ((strncmp(opcode, "ST#$00     ,Y" ,13) == 0)) {
            strcpy(opcode, "CLR");
            strcpy(operand, ",Y");
        }
        sprintf(m6809Line->lineWorkBuffer, "%s %s %s ; Z80: %s", z80Line.label, opcode, operand, z80Line.originalLine);
    }
}

void handleJP(struct asmLine z80Line, struct asmLine *m6809Line) {
    sprintf(m6809Line->lineWorkBuffer, "%s lbra %s ; Z80: %s", z80Line.label, z80Line.operand, z80Line.originalLine);
}

void handleORG(struct asmLine z80Line, struct asmLine *m6809Line) {
    sprintf(m6809Line->lineWorkBuffer, "%s org %s ; Z80: %s", z80Line.label, z80Line.operand, z80Line.originalLine);
}

void handleEQU(struct asmLine z80Line, struct asmLine *m6809Line) {
    sprintf(m6809Line->lineWorkBuffer, "%s equ %s ; Z80: %s", z80Line.label, z80Line.operand, z80Line.originalLine);
}

void handleNOP(struct asmLine z80Line, struct asmLine *m6809Line) {
    sprintf(m6809Line->lineWorkBuffer, "%s nop ; Z80: %s", z80Line.label, z80Line.originalLine);
}

void handleDI(struct asmLine z80Line, struct asmLine *m6809Line) {
    sprintf(m6809Line->lineWorkBuffer, "%s orcc #$50 ; Z80: %s", z80Line.label, z80Line.originalLine);
}

int main(int argc, char* argv[])
{
    FILE *file = NULL;
    char line[256];
    char newcode[256];
    int lnumber, count = 0;
    
    int ret,x,pos,point,found,store = 0;
    char address[256];
    char first[256];
    char second[256];
    char temp[256];
    char oldcode[256];
    char other[256] = "";
    char z80[256];
    char m6809[256] = "UNKNOWN";
    char in_out[1000][25];
    
    if (argc == 2)
       	file = fopen(argv[1], "r");
    else {
        fprintf(stderr, "error: wrong number of arguments\n"
                "usage: %s textfile\n", argv[0]);
        return 1;
    }
    
    lnumber = 1;
    while (fgets(line, sizeof(line), file)) {
        /* note that fgets don't strip the terminating \n, checking its
         presence would allow to handle lines longer that sizeof(line) */
        // remove the line feed at the end of 'line'
        line[strcspn(line, "\r")] = 0;
        line[strcspn(line, "\n")] = 0;

        struct asmLine theLine;
        parseLine(line, &theLine);
        prettyPrint(theLine);
        
    }
       
    fclose(file);
    
    return 0;
}
