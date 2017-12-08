/*
Equipe 17
Nome do projeto: MIPS-COVER
Matheus Dos Santos Luccas, 9005961, 
William Luis Alves Ferreira, 9847599, illiamw
*/

#include "cpu.h"
#include "mask.h"
//DEfinindos estados base

#define S0   0x9408
#define S1   0x0018
#define S2   0x0014
#define S3   0x1800
#define S4   0x4002
#define S5   0x0802
#define S6   0x0044
#define S7   0x0003
#define S8   0x02A4
#define S9   0x0480

char zero, overflow;

void alu_control(int IR,short int sc, char *alu_op);

int alu(int a, int b, char alu_op, int *result_alu, char *zero, char *overflow){
    //Opções da ULA
    switch(alu_op){
        case ativa_or:
            *result_alu = (a | b);
            break;
        case ativa_and:
            *result_alu = (a & b);
            break;
        case ativa_soma:
            *result_alu = (a + b);
            break;
        case ativa_subtracao:
            *result_alu = (a - b);
            break;
        case ativa_slt:
            if(a < b) *result_alu = 0x01;
            else *result_alu = 0x00;
            break;
        default:
            return (-1);
    }
    //Reset Bit Zero para condição de incremento em PC && SET bit zero 
    if(*result_alu != 0)  *zero = desativa_bit_zero;
    else *zero = ativa_bit_zero;

    return 0;
}

void control_unit(int IR, short int *sc){
    //definindo o sinal de controle inicial
    if(IR == (-1)) *sc = S0;
    else{
        //separando e delocando bits significativos do OPCODE
        char opCode = ((IR & separa_cop) >> 26);
        //DEfinindo proximo sinal de controle
        if(*sc == ((short int)S0)) *sc = S1;
        else{
            if(opCode == 0x00){
                switch(*sc){
                    case ((short int)S1):
                        *sc = S6;
                        break;
                    case ((short int)S6):
                        *sc = S7;
                        break;
                    case ((short int)S7):
                        *sc = S0;
                        break;
                }
            }

            else if(opCode == 0x02){
                switch(*sc){
                    case ((short int)S1):
                        *sc = S9;
                        break;
                    case ((short int)S9):
                        *sc = S0;
                        break;
                }
            }

            else if(opCode == 0x04){
                switch(*sc){
                    case ((short int)S1):
                        *sc = S8;
                        break;
                    case ((short int)S8):
                        *sc = S0;
                        break;
                }
            }

            else if(opCode == 0x23){
                switch(*sc){
                    case ((short int)S1):
                        *sc = S2;
                        break;
                    case ((short int)S2):
                        *sc = S3;
                        break;
                    case ((short int)S3):
                        *sc = S4;
                        break;
                    case ((short int)S4):
                        *sc = S0;
                        break;
                }
            }

            else if(opCode == 0x2b){
                switch(*sc){
                    case ((short int)S1):
                        *sc = S2;
                        break;
                    case ((short int)S2):
                        *sc = S5;
                        break;
                    case ((short int)S5):
                        *sc = S0;
                        break;
                }
            }
            return;
        }
    }
}

void instruction_fetch(short int sc, int PC, int ALUOUT, int IR, int* PCnew, int* IRnew, int* MDRnew){
    char alu_op;
    //Verificado estado para a execução
    if(sc == ((short int)S0)){
        *IRnew = memory[PC];

        alu_control(IR, sc, &alu_op);//definindo sinal de contro da ULA
        alu(PC, 1, alu_op, &ALUOUT, &zero, &overflow);//Operando na ULA

        //Passando resultados da inspeção de instruçõe
        *PCnew = ALUOUT;
        *MDRnew = memory[PC];

        if(*IRnew == 0){
			loop = 0;
		}
    }

    return;
}

void decode_register(short int sc, int IR, int PC, int A, int B, int *Anew, int *Bnew, int *ALUOUTnew){
    int auxA, auxB;
    //Definindo A e B para possivel operação na ULA
    if(sc == ((short int)S1)){
        char alu_op;

        auxA = reg[(separa_rs & IR) >> 21];
        auxB = reg[(separa_rt & IR) >> 16];

        *Anew = auxA;
        *Bnew = auxB;

        alu_control(IR, sc, &alu_op);
        alu(PC, (IR & separa_imediato), alu_op, ALUOUTnew, &zero, &overflow);
    }

    return;
}

void exec_calc_end_branch(short int sc, int A, int B, int IR, int PC, int ALUOUT, int *ALUOUTnew, int *PCnew){
    char alu_op;
    
    alu_control(IR, sc, &alu_op);

    switch(sc){
        //DEfinindo operações na ULA
        //tipo Lw/Sw
        case ((short int)S2):
            alu(A, IR & separa_imediato, alu_op, ALUOUTnew, &zero, &overflow);
            break;

        //tipo-R
        case ((short int)S6):
            alu(A, B, alu_op, ALUOUTnew, &zero, &overflow);
            break;

        //tipo J
        case ((short int)S9):
            *PCnew = ((separa_endereco_jump & IR) | (separa_4bits_PC & PC));
            break;

        // BEQ
        case ((short int)S8):
            alu(A, B, alu_op, ALUOUTnew, &zero, &overflow);
            if(zero == 1){
                *PCnew = ALUOUT;
            }else return;
            break;

        default:
            break;
    }

    return;
}

void write_r_access_memory(short int sc, int B, int IR, int ALUOUT, int PC, int *MDRnew, int *IRnew){
    //Escrevendo resultados
    //tipo Lw
    if(sc == (short int)S3){
        *MDRnew = memory[ALUOUT];
    }
    //tipo Sw
    else if(sc == (short int)S5){
        memory[ALUOUT] = reg[(IR & separa_rt) >> 16];
    }
    //tipo R
    else if(sc == (short int)S7){
        reg[(IR & separa_rd) >> 11] = ALUOUT;
    }
}

void write_ref_mem(short int sc, int IR, int MDR, int ALUOUT){
    
    if(sc == ((short int)S4)){
        reg[(IR & separa_rt) >> 16] = MDR;
    }
}

void alu_control(int IR,short int sc, char *alu_op){
    //Definindo operações na ULA atráves do opCODE
    if((((sc & separa_ALUOp0) | (sc & separa_ALUOp1)) >> 5) == 0x2){
        switch(IR & 0x0f){
            case 0b0100:
                *alu_op = 0b0000;
                break;
            case 0b0101:
                *alu_op = 0b0001;
                break;
            case 0b0000:
                *alu_op = 0b0010;
                break;
            case 0b0010:
                *alu_op = 0b0110;
                break;
            case 0b1010:
                *alu_op = 0b0111;
                break;
            default:
                break;
        }

    } else if((((sc & separa_ALUOp0) | (sc & separa_ALUOp1)) >> 5) == 0x3){
        switch(IR & 0x0f){
            case 0b0010:
                *alu_op = 0b0110;
                break;
            case 0b1010:
                *alu_op = 0b0111;
                break;
            default:
                break;
        }

    } else if((((sc & separa_ALUOp0) | (sc & separa_ALUOp1)) >> 5) == 0x0){
        *alu_op = 0b0010;

    } else if((((sc & separa_ALUOp0) | (sc & separa_ALUOp1)) >> 5) == 0x1){
        *alu_op = 0b0110;
    }
        return;
}
