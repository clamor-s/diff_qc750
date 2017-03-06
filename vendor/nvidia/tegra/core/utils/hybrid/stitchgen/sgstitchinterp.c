
#include "sgstitchinterp.h"
#include "hgdefs.h"

#if defined(IN_REGRESSION_TESTER)
#   include <stdlib.h>
#   include <stdio.h>
#endif

#if (HG_COMPILER == HG_COMPILER_RVCT)
#   include "nvos.h"
#   define MEMCPY(source1, source2, size) NvOsMemcpy(source1, source2, size)
#else
#   include <string.h>
#   define MEMCPY(source1, source2, size) memcpy(source1, source2, size)
#endif

/* Low values (32-64) are OK if no function calls are needed. */
#define MAX_EVAL_STACK_DEPTH    64
#define MAX_PATCHABLE_JUMPS     64
#define MAX_LABEL_POSITIONS     128
#define MAX_VARS                64  /* Maximum number of !vars */
#define MAX_CALL_DEPTH          16  /* Maximum call nesting (used for IP reg save/restore) */

/* HAS TO BE BIGGER THAN MAX LABEL NDX IN INPUT PROGRAM */
#define LABEL_OFFSET_INCREMENT 256

HG_CT_ASSERT(MAX_PATCHABLE_JUMPS < LABEL_OFFSET_INCREMENT);

typedef enum
{
    BC_BZ,
    BC_BNZ,
    BC_BRA,
    BC_PUSH_ST1,
    BC_POP_ST0,
    BC_SET_VAR,
    BC_GET_VAR,
    BC_SET_LOCAL_VAR,
    BC_GET_LOCAL_VAR,
    BC_PUSHA_S8,
    BC_PUSHA_U8,
    BC_PUSHA_S16,
    BC_PUSHA_U16,
    BC_PUSHA_S32,
    BC_PUSHA_U32,
    BC_RECORD_EMIT_LABEL,
    BC_PATCH_LABEL_DIFF,
    BC_EMIT,
    BC_PUSHC,
    BC_BOP_ADD,
    BC_BOP_SUB,
    BC_BOP_MUL,
    BC_BOP_DIV,
    BC_BOP_BITWISE_AND,
    BC_BOP_BITWISE_OR,
    BC_BOP_BITWISE_XOR,
    BC_BOP_LSL,
    BC_BOP_LSR,
    BC_BOP_ASR,
    BC_BOP_BOOL_AND,
    BC_BOP_BOOL_OR,
    BC_BOP_LT,
    BC_BOP_LTE,
    BC_BOP_GT,
    BC_BOP_GTE,
    BC_BOP_EQ,
    BC_BOP_NEQ,
    BC_UOP_BOOL_NOT,
    BC_CASE_CMP,
    BC_CASE_CMP_BS,
    BC_TRAP,
    BC_COMMENT,
    BC_EOP,
    BC_PUSH_ARR_A_S8,
    BC_PUSH_ARR_A_U8,
    BC_PUSH_ARR_A_S16,
    BC_PUSH_ARR_A_U16,
    BC_PUSH_ARR_A_S32,
    BC_PUSH_ARR_A_U32,
    BC_CALL,
    BC_ENTER,
    BC_RET,
    BC_PATCH_IMMEDIATE,
    BC_PATCH_IMMEDIATE_ARM_12B,
    BC_PATCH_IMMEDIATE_ARM_SIGNED_ADDR_12,
    BC_PATCH_IMMEDIATE_ARM_S7_L5,
    BC_PATCH_IMMEDIATE_ARM_SIGNED_ADDR_8,
    BC_PATCH_IMMEDIATE_RESERVED2,
    BC_PATCH_IMMEDIATE_RESERVED3,
    BC_PATCH_IMMEDIATE_RESERVED4,
    BC_PUSHC_0,
    BC_PUSHC_1,
    BC_PUSHC_2,
    BC_PUSHC_3,
    BC_PUSHC_4,
    BC_PUSHC_5,
    BC_PUSHC_6,
    BC_PUSHC_7,
    BC_PUSHC_8,
    BC_PUSHC_9,
    BC_PUSHC_10,
    BC_PUSHC_11,
    BC_PUSHC_12,
    BC_PUSHC_13,
    BC_PUSHC_14,
    BC_PUSHC_15,
    BC_PUSHC_16,
    BC_PUSHC_17,
    BC_PUSHC_18,
    BC_PUSHC_19,
    BC_PUSHC_20,
    BC_PUSHC_21,
    BC_PUSHC_22,
    BC_PUSHC_23,
    BC_PUSHC_24,
    BC_PUSHC_25,
    BC_PUSHC_26,
    BC_PUSHC_27,
    BC_PUSHC_28,
    BC_PUSHC_29,
    BC_PUSHC_30,
    BC_PUSHC_31
} Opcode;

static const HGuint8 opInfo[] = 
{
    1|(1<<4),
    1|(0<<4),
    0|(1<<4),
    1|(1<<4),
    1|(2<<4),
    2|(2<<4),
    2|(2<<4),
    2|(2<<4),
    4|(1<<4),
    1|(0<<4),
    0|(0<<4),
    0|(0<<4),
    0|(0<<4),
    0|(0<<4),
    0|(0<<4),
    0|(0<<4),
    0|(0<<4),
    0|(0<<4),
    0|(0<<4),
    1|(1<<4),
    1|(1<<4),
    0|(1<<4),
    1|(1<<4),
    1|(1<<4),
    1|(1<<4),
    1|(0<<4),
    3|(1<<4),
    1|(1<<4),
    1|(1<<4),
    1|(1<<4),
    0|(0<<4),
    0|(0<<4),
    0|(0<<4),
    0|(0<<4),
    0|(0<<4),
    0|(0<<4),
    0|(0<<4),
    0|(0<<4),
    0|(0<<4),
    0|(0<<4),
    0|(0<<4),
    0|(0<<4),
    0|(0<<4),
    0|(0<<4),
    0|(0<<4),
    0|(0<<4)
};

HG_INLINE int getOpNargs(int op)
{
    int ndx    = op >> 1;
    int nibble = (op & 1);
    return (opInfo[ndx] >> (nibble<<2)) & 15;
}

enum
{
    BC_NS_STITCH_CONST = 0,
    BC_NS_STITCH_TIME = 1,
    BC_NS_RUN_TIME = 2,
    BC_NS_MAX
};

typedef struct
{
    HGuint16 progPos;
    HGuint16 label;
    HGuint8  bitStart;
    HGuint8  bitLength;
} BraPatch;

HG_CT_ASSERT(sizeof(BraPatch) == 6 || sizeof(BraPatch) == 8);

typedef struct
{
    HGuint16 pos;
    HGuint16 label;
} LabelPos;

typedef struct
{
    const void* savedIP;
    HGint32     labelOffs;
    HGint32     frameSize;
} CallRecord;

/* If we're compiling stitchgen regression tester, then use a
   different mechanism for trap: */
#if defined(IN_REGRESSION_TESTER)
HG_STATICF void trap(int line)
{
    fprintf(stderr, "Interpreter trap: !assert on line %i\n", line+1);
    abort();
}
#   define TRAP(x) trap(line)
#else
#   define TRAP(x) HG_UNREF(x); HG_ASSERT(!"Stitchgen !assert failed!");
#endif


/* Used for stringifying version numbers: */ 
#define xstr(s) str(s) 
#define str(s) #s 

/*-------------------------------------------------------------------*//*!
 * \brief   Return HG's version number as a C string
 *//*-------------------------------------------------------------------*/
const char* sgGetVersion(void) 
{ 
    const char* const s = 
        xstr(SG_VERSION_MAJOR) "." 
        xstr(SG_VERSION_MINOR) "." 
        xstr(SG_VERSION_PATCH); 
    return s; 
}

/* Write little endian integer */
HG_STATICF void writeInt(HGuint8* b, HGuint32 i)
{
    b[0] = (HGuint8)(i & 0xff);
    b[1] = (HGuint8)((i >> 8) & 0xff);
    b[2] = (HGuint8)((i >> 16) & 0xff);
    b[3] = (HGuint8)((i >> 24) & 0xff);
}

/* NOTE: this could be HG_INLINE on RVCT, but produces significantly
   worse code on ARM GCC.  Thus outline. */
HG_NOINLINE HG_STATICF int readCompressedInt(const HGuint8** pB)
{
    int            v;
    HGuint8        x0, x;
    const HGuint8* b = *pB;

    x0 = *b++;
    v = x0 & 0x7f;
    if (x0 & 0x40)
    {
        int shift = 6;

        do
        {
            x = *b++;
            v ^= (x << shift);
            shift += 7;
        } while(x & 0x80);
    }
    if (x0 & 0x80)
        v = -v;
    *pB = b;
    return v;
}

HG_STATICF int readInt(const HGuint8* b)
{
    return b[0] | (b[1]<<8) | (b[2]<<16) | (b[3]<<24);
}

HG_STATICF HGuint32 setBits(HGuint32 orig, int start, int len, HGuint32 new)
{
    int mask;
    if(len == 32)
        return new;
    mask = ((1 << len)-1) << start;
    return (orig & ~mask) | ((new&((1<<len)-1))<<start);
}


HG_STATICF HG_NOINLINE int evalBinop(int op, int arg0, int arg1)
{
    switch(op)
    {
    case BC_BOP_ADD:            return arg0+arg1;
    case BC_BOP_SUB:            return arg0-arg1;
    case BC_BOP_MUL:            return arg0*arg1;
    case BC_BOP_DIV:    HG_ASSERT(arg1 != 0); return arg0/arg1;
    case BC_BOP_BITWISE_AND:    return arg0 & arg1;
    case BC_BOP_BITWISE_OR:     return arg0 | arg1;
    case BC_BOP_BITWISE_XOR:    return arg0 ^ arg1;
    case BC_BOP_LSL:            return arg0 << arg1;
    case BC_BOP_LSR:            return (HGuint32)arg0 >> (HGuint32)arg1;
    case BC_BOP_ASR:            return (HGuint32)((HGint32)arg0 >> (HGint32)arg1);
    case BC_BOP_BOOL_AND:       return arg0 && arg1;
    case BC_BOP_BOOL_OR:        return arg0 || arg1;
    case BC_BOP_LT:             return arg0 < arg1;
    case BC_BOP_LTE:            return arg0 <= arg1;
    case BC_BOP_GT:             return arg0 > arg1;
    case BC_BOP_GTE:            return arg0 >= arg1;
    case BC_BOP_EQ:             return arg0 == arg1;
    case BC_BOP_NEQ:            return arg0 != arg1;
    default: HG_ASSERT(0);
    }
    return 0;
}

HG_INLINE int evalUnop(int op, int arg0)
{
    switch(op)
    {
    case BC_UOP_BOOL_NOT:   return !arg0;
    default: HG_ASSERT(0);
    }
    return 0;
}

HG_INLINE HGuint32 rol32(HGuint32 v, HGuint32 sh)
{
    return (v << sh) | (v >> (32-sh));
}

/*-------------------------------------------------------------------*//*!
 * \brief   Fit a 32-bit int into ARM 12-bit ROL'd representation
 * \param   v   Value to be transformed
 * \return  ARM 12-bit ROL'd encoding of the original value.
 *
 * This function is used to encode generic 32-bit values into ARM's
 * 12-bit ROL'd representation.  These values are used in instructions
 * such as "mov r0,#0xff00".
 *
 * Obviously not all 32-bit integers fit into this encoding.  If such
 * values are encountered, it means there is a programming mistake in
 * the original stitcher.  It's the stitcher's responsibility to
 * handle such values correctly; it's impossible to do that here.
 *//*-------------------------------------------------------------------*/

HG_INLINE HGuint32 encodeARM12BitsImmediate(HGuint32 v)
{
    int rol = 0;
    while((v & ~255) != 0)
    {
        rol++;
        HG_ASSERT(rol < 16);
        v = rol32(v, 2);
    }
    return v | (rol<<8);
}

HG_INLINE HGint32 popVal(HGint32** evalStack, HGint32* framePtr)
{
    HGint32  v;
    HGint32* es = *evalStack - 1;
#if defined(HG_DEBUG)
    HG_ASSERT(*evalStack < framePtr);
#else
    HG_UNREF(framePtr);
#endif
    v = *es;
    *evalStack = es;
    return v;
}

HG_INLINE void pushVal(HGint32** evalStack, HGint32* framePtr, HGint32 v)
{
    HGint32* es = *evalStack;
#if defined(HG_DEBUG)
    HG_ASSERT(*evalStack < framePtr);
#else
    HG_UNREF(framePtr);
#endif
    *es = v;
    *evalStack = es+1;
}

HG_INLINE void addLabelPos(LabelPos* labelPos, int* nLabels, int pos, int label)
{
    LabelPos p;

    HG_ASSERT(pos < 65536);
    HG_ASSERT(label < 65536);

    p.pos   = (HGuint16)pos;
    p.label = (HGuint16)label;

    labelPos[*nLabels] = p;
    *nLabels = *nLabels + 1;
}

HG_STATICF int findLabelPos(LabelPos* labelPos, int nLabels, int label)
{
    int i = nLabels - 1;

    HG_ASSERT(nLabels > 0);

    do
    {
        if((int)labelPos[i].label == label)
            return labelPos[i].pos;

        i--;
    } while(i >= 0);
    HG_ASSERT(0);
    return -1;
}

typedef struct
{
    HGuint8 startBit;
    HGuint8 length;
} ImmPatchInfo;

HG_STATICF HG_NOINLINE void 
evalImmediatePatch(int      op,
                   int      prevEmitStart,
                   void*    dst,
                   void*    maxDestPtr,
                   HGint32* opArgs,
                   HGuint32 v)
{
    static const ImmPatchInfo patchInfo[] = 
    {
        { 0, 0 },       /* BC_PATCH_IMMEDIATE */
        { 0, 12 },      /* BC_PATCH_IMMEDIATE_ARM_12B */
        { 0, 12 },      /* BC_PATCH_IMMEDIATE_ARM_IMMEDIATE_SIGNED_ADDR_12 */
        { 7, 5 },       /* BC_PATCH_IMMEDIATE_ARM_S7_L5 */
        { 0, 4 },       /* BC_PATCH_IMMEDIATE_ARM_IMMEDIATE_SIGNED_ADDR_8 */
    };

    HGuint32  ins;
    int       progPos = prevEmitStart + opArgs[0];
    HGuint8*  insptr  = (HGuint8*)dst + progPos;
    int       startBit;
    int       length;

    HG_UNREF(maxDestPtr);

    HG_ASSERT((HGintptr)insptr < (HGintptr)maxDestPtr);

    /* Generic patch: */
    if(op == BC_PATCH_IMMEDIATE)
    {
        startBit = opArgs[1];
        length   = opArgs[2];
    } else
    {
        /* CPU specific patch */
        const ImmPatchInfo* p = &patchInfo[op - BC_PATCH_IMMEDIATE];

        startBit = p->startBit;
        length   = p->length;
    }

    ins = readInt(insptr);

    switch(op)
    {
    case BC_PATCH_IMMEDIATE:
    case BC_PATCH_IMMEDIATE_ARM_S7_L5:
        break;
    case BC_PATCH_IMMEDIATE_ARM_12B:
        v = encodeARM12BitsImmediate(v);
        break;

   case BC_PATCH_IMMEDIATE_ARM_SIGNED_ADDR_8:
   case BC_PATCH_IMMEDIATE_ARM_SIGNED_ADDR_12:
       {
           int bv = 1;

           /* Toggle ARM ldr/str address mode 2/3 sign bit: */
           if((HGint32)v < 0)
           {
               v  = -(HGint32)v;
               bv = 0;
           }
           ins = setBits(ins, 23, 1, bv);

           if (op == BC_PATCH_IMMEDIATE_ARM_SIGNED_ADDR_8)
               ins = setBits(ins, 8, 4, v >> 4);
       }
       break;
    default:
        HG_ASSERT(0);
    }

    writeInt(insptr, setBits(ins, startBit, length, v));
}

HG_NOINLINE HG_STATICF 
sgReturn
sgInterpretStitcherInternal(void* dst,
                            int   maxDestSize,
                            const void* inputProgram,
                            const void* stitchTimeArgs,
                            const void* stitchConstArgs,
                            const char* const* commentStrings,
                            sgComment* outputComments,
                            int maxOutputComments,
                            HGint32* vars,
                            LabelPos* labelPos,
                            BraPatch* jumpPatches)
{
#define PUSH(x) pushVal(&esp, framePtr, x)
#define POP()   popVal(&esp, framePtr)
#define PUSH_CALL_RECORD(x) *(callStackSp++) = (x)
#define POP_CALL_RECORD() *(--callStackSp)
    HGint32        evalStack[MAX_EVAL_STACK_DEPTH];
    CallRecord     callStack[MAX_CALL_DEPTH];

    const HGuint8* bytecode        = (const HGuint8*)inputProgram + 1;
    int            nRecordedLabels = 0;
    int            prevEmitStart   = 0;
    int            nComments       = 0;
    int            nJumpPatches    = 0;
    int            masterLabelOffset  = 0;
    int            curLabelOffset  = 0;
    HGint32*       esp             = evalStack;
    HGint32*       framePtr        = evalStack + MAX_EVAL_STACK_DEPTH - 1;
    CallRecord*    callStackSp     = callStack;
    HGuint8*       output          = dst;
    HGuint8*       maxDestPtr      = (HGuint8*)dst + maxDestSize;
    sgReturn       retVal;
    const void*    nsArgs[2];

    nsArgs[0] = stitchConstArgs;
    nsArgs[1] = stitchTimeArgs;

    retVal.outputSize      = SG_STITCH_MAX_SIZE_EXCEEDED;
    retVal.nOutputComments = SG_STITCH_MAX_SIZE_EXCEEDED;

    for(;;)
    {
        int      i;
        int      nArgs;
        int      args[4];

        HGuint32       opbyte = *bytecode++;
        const HGuint8* prevBC;

        HG_ASSERT(esp >= evalStack && esp < evalStack + MAX_EVAL_STACK_DEPTH);

        /* Instructions: const_0 through const_31 */
        if(opbyte >= BC_PUSHC_0)
        {
            HG_ASSERT(opbyte <= BC_PUSHC_31);
            PUSH((int)opbyte - BC_PUSHC_0);
            continue;
        }

        if(opbyte >= BC_BOP_ADD && opbyte <= BC_BOP_NEQ)
        {
            /* Note: reversed order of arguments is intentional. */
            int arg1 = POP();
            int arg0 = POP();
            PUSH(evalBinop(opbyte, arg0, arg1));
            continue;
        }

        prevBC = bytecode;

        nArgs = getOpNargs(opbyte);
        for(i = 0; i < nArgs; i++)
        {
            HG_ASSERT(i < (int)(sizeof(args)/sizeof(args[0])));
            args[i] = readCompressedInt(&bytecode);
        }

        switch(opbyte)
        {
        case BC_BZ:
        case BC_BNZ:
            {
                int v = POP();

                HG_ASSERT((opbyte - BC_BZ) == (HGuint32)(opbyte == BC_BNZ));
                
                if((v == 0) ^ (opbyte - BC_BZ))
                    bytecode = args[0] + prevBC;
                break;
            }

        case BC_BRA:
            bytecode = args[0] + prevBC;
            break;

        case BC_CALL:
            {
                CallRecord     cr;
                HG_ASSERT(curLabelOffset < 65536);

                cr.savedIP   = bytecode;
                cr.labelOffs = curLabelOffset;
                cr.frameSize = 0;

                HG_ASSERT(callStackSp < callStack + MAX_CALL_DEPTH - 1);
                PUSH_CALL_RECORD(cr);

                bytecode = args[0] + prevBC;

                /* Offset "label namespace counter" for the call.
                   This way labels generated from inside a function
                   will have different indices for each invocation of
                   the function. */
                masterLabelOffset += LABEL_OFFSET_INCREMENT;
                curLabelOffset     = masterLabelOffset;
            }
            break;

        case BC_ENTER:
            {
                int frameSize = args[0];
                /* Enter new local variable frame */

                HG_ASSERT(callStackSp - 1 >= callStack);
                HG_ASSERT(callStackSp < callStack + MAX_CALL_DEPTH);
                HG_ASSERT((callStackSp-1)->frameSize == 0);

                framePtr -= frameSize;
                (callStackSp-1)->frameSize = frameSize;
            }
            break;

        case BC_RET:
            {
                CallRecord cr;

                HG_ASSERT(callStackSp > callStack);
                HG_ASSERT(callStackSp < callStack + MAX_CALL_DEPTH);

                /* Return & adjust local frame offset */
                cr = POP_CALL_RECORD();

                framePtr       += cr.frameSize;
                curLabelOffset  = cr.labelOffs;
                bytecode        = cr.savedIP;
            }
            break;

        case BC_PUSH_ST1:
            {
                HGint32 value;
                HG_ASSERT(esp >= evalStack+2);
                value = esp[-2];
                PUSH(value);
            }
            break;
            /* Pop the value off the stack (and discard) */
        case BC_POP_ST0:
            {
                HG_ASSERT(esp > evalStack);
                esp--;
            }
            break;

        case BC_SET_VAR:
        case BC_GET_VAR:
            {
                int      ndx = args[0];
                HGint32* v    = vars + ndx;

                HG_ASSERT(ndx >= 0 && ndx < MAX_VARS);

                if(opbyte == BC_SET_VAR)
                {
                    *v = POP();
                } else
                {
                    HG_ASSERT(opbyte == BC_GET_VAR);
                    PUSH(*v);
                }
            }
            break;

        case BC_SET_LOCAL_VAR:
        case BC_GET_LOCAL_VAR:
            {
                int      ndx = args[0];
                HGint32* fp  = framePtr + ndx;

                HG_ASSERT(fp < evalStack + MAX_EVAL_STACK_DEPTH);
                HG_ASSERT(fp > esp);
                
                if(opbyte == BC_SET_LOCAL_VAR)
                {
                    *fp = POP();
                } else
                {
                    HG_ASSERT(opbyte == BC_GET_LOCAL_VAR);
                    PUSH(*fp);
                }
            }
            break;

        case BC_PUSHA_U8:
        case BC_PUSHA_S8:
        case BC_PUSHA_U16:
        case BC_PUSHA_S16:
        case BC_PUSHA_U32:
        case BC_PUSHA_S32:
            {
                int      ns     = args[0];
                int      offs   = args[1];
                HGintptr argPtr = (HGintptr)nsArgs[ns]+offs;
                int      v      = 0;

                HG_ASSERT(ns >= 0 && ns <= BC_NS_STITCH_TIME);

#define READ_ARG(ty) *((const ty*)(argPtr))
                switch(opbyte)
                {
                case BC_PUSHA_S8:  v = READ_ARG(HGint8);   break;
                case BC_PUSHA_U8:  v = READ_ARG(HGuint8);  break;
                case BC_PUSHA_S16: v = READ_ARG(HGint16);  break;
                case BC_PUSHA_U16: v = READ_ARG(HGuint16); break;
                case BC_PUSHA_S32: v = READ_ARG(HGint32);  break;
                case BC_PUSHA_U32: v = READ_ARG(HGuint32); break;
                default: HG_ASSERT(0);
                }
                PUSH(v);
                break;
#undef READ_ARG
            }
        case BC_PUSH_ARR_A_U8:
        case BC_PUSH_ARR_A_S8:
        case BC_PUSH_ARR_A_U16:
        case BC_PUSH_ARR_A_S16:
        case BC_PUSH_ARR_A_U32:
        case BC_PUSH_ARR_A_S32:
            {
                int      arrOffs = POP();
                int      offs    = args[0];
                int      v       = 0;
                HGintptr argsPtr = (HGintptr)stitchTimeArgs+offs;
#define READ_ARG(ty) ((*((const ty* const*)(argsPtr)))[arrOffs])
                
                switch(opbyte)
                {
                case BC_PUSH_ARR_A_S8:  v = READ_ARG(HGint8);   break;
                case BC_PUSH_ARR_A_U8:  v = READ_ARG(HGuint8);  break;
                case BC_PUSH_ARR_A_S16: v = READ_ARG(HGint16);  break;
                case BC_PUSH_ARR_A_U16: v = READ_ARG(HGuint16); break;
                case BC_PUSH_ARR_A_S32: v = READ_ARG(HGint32);  break;
                case BC_PUSH_ARR_A_U32: v = READ_ARG(HGuint32); break;
                default: HG_ASSERT(0);
                }
                PUSH(v);
                break;
#undef READ_ARG
            }
        case BC_RECORD_EMIT_LABEL:
            {
                int lbl = args[0];
                int pos = args[1];

                HG_ASSERT(lbl < LABEL_OFFSET_INCREMENT && "validate input bytecode");

                lbl += curLabelOffset;

                HG_ASSERT(nRecordedLabels < MAX_LABEL_POSITIONS);

                addLabelPos(labelPos, &nRecordedLabels, prevEmitStart + pos, lbl);
                break;
            }
        case BC_PATCH_LABEL_DIFF:
            {
                BraPatch p;
                int      lbl     = args[0];
                int      progPos = prevEmitStart + args[1];

                HG_ASSERT(lbl < LABEL_OFFSET_INCREMENT && "validate input bytecode");

                lbl += curLabelOffset;

                p.progPos   = (HGuint16)progPos;
                p.label     = (HGuint16)lbl;
                p.bitStart  = (HGuint8)args[2];
                p.bitLength = (HGuint8)args[3];

                HG_ASSERT(p.bitStart < 32);
                HG_ASSERT(p.bitLength <= 32);

                HG_ASSERT(nJumpPatches < MAX_PATCHABLE_JUMPS);
                jumpPatches[nJumpPatches++] = p;
                break;
            }
        case BC_PATCH_IMMEDIATE:
        case BC_PATCH_IMMEDIATE_ARM_12B:
        case BC_PATCH_IMMEDIATE_ARM_SIGNED_ADDR_12:
        case BC_PATCH_IMMEDIATE_ARM_S7_L5:
        case BC_PATCH_IMMEDIATE_ARM_SIGNED_ADDR_8:
            {
                HGuint32 v = POP();
                evalImmediatePatch(opbyte,
                                   prevEmitStart, dst, maxDestPtr, args, v);
                break;
            }
        case BC_EMIT:
            {
                int length = args[0];

                if((HGintptr)output + length > (HGintptr)maxDestPtr)    
                    return retVal;

                prevEmitStart = (HGintptr)output - (HGintptr)dst;
                MEMCPY(output, bytecode, length);

                output   += length;
                bytecode += length;
                break;
            }
        case BC_PUSHC:
            {
                PUSH(args[0]);
                break;
            }

        case BC_UOP_BOOL_NOT:
            {
                /* Note: reversed order of arguments is
                   intentional. */
                int arg0 = POP();
                PUSH(evalUnop(opbyte, arg0));
                break;
            }

        case BC_CASE_CMP:
            /* Special instruction to make case-of statements smaller
               in size. Same as the sequence of:

                   push_st 1
                   pushc n     where n >= 0 && n < 256
                   eq
                   bool_or */

            {
                int arg1 = args[0];
                HG_ASSERT(esp >= evalStack+2);
                /* push_st 1 */
                /* eq */
                /* bool_or */
                if(esp[-2] == arg1)
                    esp[-1] = HG_TRUE;
            }
            break;

        case BC_CASE_CMP_BS:
            /* Special instruction to make case-of statements smaller
               in size. This works the same way as an array of
               BC_CASE_CMPs, but stored in a compact bitfield thusly:

               BC_CASE_CMP a
               BC_CASE_CMP b
               ...
               BC_CASE_CMP n

               =>

               BC_CASE_CMP_BS {a,b,...,n} where the set is stored as a
               max 32 element bitset. */

            {
                int      s1;
                int      s2;
                int      elem   = 0;
                HGuint32 bitset = (HGuint32)args[0];

                HG_ASSERT(esp >= evalStack+2);
                HG_ASSERT(bitset != 0); /* Set cannot be empty */

                s1 = esp[-1];
                s2 = esp[-2];

                do
                {
                    if((bitset & 1) && s2 == elem)
                    {
                        s1 = HG_TRUE;
                        break;
                    }

                    bitset >>= 1;
                    elem++;
                } while(bitset);

                esp[-1] = s1;
            }
            break;

        case BC_TRAP:
            {
                int line = args[0];
                TRAP(line);
                break;
            }
        case BC_COMMENT:
            {
                /* Add a "dynamic comment" into output.  This is used
                   for debugging output assembly. */
                int ndx = args[0];
                if(outputComments && commentStrings && nComments < maxOutputComments)
                {
                    sgComment c;
                    c.comment    = commentStrings[ndx];
                    c.codeOffset = (int)((HGintptr)output - (HGintptr)dst);
                    outputComments[nComments++] = c;
                }
                break;
            }
        case BC_EOP: goto eop;
        default:
            /* Unknown instruction */
            HG_ASSERT(0);
        }
    }
 eop:;
#undef POP
#undef PUSH

    /* Mustn't have any left-overs on the stack: */
    HG_ASSERT(esp == evalStack);

    /* Patch labels */
    {
        int i;

        for(i = 0; i < nJumpPatches; i++)
        {
            BraPatch  p      = jumpPatches[i];
            int       lblPos = findLabelPos(labelPos, nRecordedLabels, p.label);
            HGuint8*  insptr = ((HGuint8*)dst + p.progPos);
            int       diff;
            HGuint32  ins;

            int targetCPU = (int)*((const HGuint8*)inputProgram);
            /* NOTE: this assertion will be triggered for some types
               of (broken) input programs.  If an assembly label is
               defined inside a conditional (!if or !case) and some
               other part of code is jumping into that (using a
               different condition), this assertion might get
               triggered.  Checking this inside stitchgen compiler is
               non-trivial.  See manual for an example. */
            HG_ASSERT(lblPos >= 0);

            if(targetCPU == SG_STITCH_TARGET_ARM)
                diff = (lblPos - p.progPos - 8)>>2;
            else if(targetCPU == SG_STITCH_TARGET_X86)
            {
                diff = lblPos - p.progPos - 4;
            } else
            {
                HG_ASSERT(targetCPU == SG_STITCH_TARGET_VC);
                HG_ASSERT(HG_FALSE); /* SHOULD NOT BE HERE! */
                diff = 0;
            }

            ins = readInt(insptr);
            writeInt(insptr, setBits(ins, p.bitStart, p.bitLength, 
                                     (HGuint32)diff));
        }
    }

    retVal.outputSize = (int)((HGintptr)output - (HGintptr)dst);
    retVal.nOutputComments = nComments;
    return retVal;
}

#define TEMP_BUF_SIZE \
    (sizeof(HGint32)*MAX_VARS+sizeof(LabelPos)*MAX_LABEL_POSITIONS +   \
     sizeof(BraPatch)*MAX_PATCHABLE_JUMPS)

/**
 * How much temporary memory to allocate for stitch interpreter.  Use
 * if you want to allocate the temporary stack buffers from heap.
 */
size_t sgInterpTempMemSize(void)
{
    return TEMP_BUF_SIZE;
}

/** 
    tempMem needs to be allocated to hold exactly
    sgInterpStitchTempMemSize() bytes.
*/
sgReturn
sgInterpretStitcherExtMem(void* dst,
                          int   maxDestSize,
                          const void* inputProgram,
                          const void* stitchTimeArgs,
                          const void* stitchConstArgs,
                          const char* const* commentStrings,
                          sgComment* outputComments,
                          int maxOutputComments,
                          void* tempMem)
{
    HGuint8*  t = tempMem;
    HGint32*  vars;
    LabelPos* labelPos;
    BraPatch* jumpPatches;

    vars = (HGint32*)t;
    t += sizeof(HGint32)*MAX_VARS;

    labelPos = (LabelPos*)t;
    t += sizeof(LabelPos)*MAX_LABEL_POSITIONS;

    jumpPatches = (BraPatch*)t;

    HG_ASSERT(t + sizeof(BraPatch)*MAX_PATCHABLE_JUMPS == 
              (HGuint8*)tempMem + sgInterpTempMemSize());

    return sgInterpretStitcherInternal(dst, maxDestSize, inputProgram, 
                                       stitchTimeArgs, stitchConstArgs,
                                       commentStrings, outputComments,
                                       maxOutputComments,
                                       vars, labelPos, jumpPatches);
}

sgReturn
sgInterpretStitcher(void* dst,
                    int   maxDestSize,
                    const void* inputProgram,
                    const void* stitchTimeArgs,
                    const void* stitchConstArgs,
                    const char* const* commentStrings,
                    sgComment* outputComments,
                    int maxOutputComments)
{
    /* Allocate some stack arrays in this wrapper to make the
       interpreter stack frame smaller.  This makes Thumb compiled
       code smaller. */

    HGuint8 tempBuf[TEMP_BUF_SIZE];
    return sgInterpretStitcherExtMem(dst, maxDestSize, inputProgram, 
                                     stitchTimeArgs, stitchConstArgs,
                                     commentStrings, outputComments,
                                     maxOutputComments,
                                     tempBuf);
}

void sgPackSignature(void* dst, 
                     const void* stitchTimeArgs,
                     const void* compactString)
{
    int            outBitPos = 0;
    HGuint8*       d = (HGuint8*)dst;
    const HGuint8* s = stitchTimeArgs;
    const HGuint8* p = compactString;

    for(;;)
    {
        int      i;
        HGint32  v;
        int      cSize;
        int      oWidth;
        HGuint32 insn = *p++;

        if(!insn)
            break;

        cSize  = insn & 3;
        oWidth = insn >> 2;

        HG_ASSERT(oWidth > 0 && oWidth <= 32);

        /* Note: 's' is always properly aligned, as the input data is
           sorted in decreasing order of cSize. */
        switch(cSize)
        {
        case 0:  v = *((const HGuint8*)s);  break;
        case 1:  v = *((const HGuint16*)(const void*)s); break;
        case 2:  v = *((const HGuint32*)(const void*)s); break;
        default: v = 0; HG_ASSERT(HG_FALSE);
        }
        s += 1 << cSize;

        /* TODO optimize: remove redundant memory loads? */
        i = 0;
        do
        {
            int byteOffs = outBitPos >> 3;
            int bitOffs  = outBitPos & 7;

            d[byteOffs] = (HGuint8)((d[byteOffs] & ~(1<<bitOffs)) | (((v >> i)&1) << bitOffs));

            outBitPos++;
            i++;
        } while(i != oWidth);
    }
}

void sgUnpackSignature(void* dst, 
                       const void* stitchTimeArgs,
                       const void* compactString)
{
    int           inBitPos = 0;
    void*          d = dst;
    const HGuint8* s = stitchTimeArgs;
    const HGuint8* p = compactString;

    for(;;)
    {
        int      i;
        HGint32  v;
        int      cSize;
        int      oWidth;
        HGuint32 insn = *p++;

        if(!insn)
            break;

        cSize  = insn & 3;
        oWidth = insn >> 2;

        HG_ASSERT(oWidth > 0 && oWidth <= 32);
        
        /* TODO optimize: remove redundant memory loads? */
        i = 0;
        v = 0;
        do
        {
            int byteOffs = inBitPos >> 3;
            int bitOffs  = inBitPos & 7;

            v |= ((s[byteOffs] >> bitOffs) & 1) << i;
            inBitPos++;
            i++;
        } while(i != oWidth);

        /* Note: 's' is always properly aligned, as the input data is
           sorted in decreasing order of cSize. */
        switch(cSize)
        {
        case 0:  *((HGuint8*)d)  = (HGuint8)v;  break;
        case 1:  *((HGuint16*)d) = (HGuint16)v; break;
        case 2:  *((HGuint32*)d) = v;           break;
        default: HG_ASSERT(HG_FALSE);
        }
        d = (void*)((HGintptr)d + (1 << cSize));
    }
}
