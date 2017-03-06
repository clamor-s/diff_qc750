#ifndef __SGSTITCHINTERP_H
#define __SGSTITCHINTERP_H

#if !defined(__SGDEFS_H)
#   include "sgdefs.h"
#endif

#define SG_STITCH_MAX_SIZE_EXCEEDED (-1)

typedef enum
{
    SG_STITCH_TARGET_DEBUG,
    SG_STITCH_TARGET_ARM,
    SG_STITCH_TARGET_X86,
    SG_STITCH_TARGET_VC
} sgStitchTargetCPU;

/** A structure used to communicate dynamic comment data to the stitch
    interpreter user. */
typedef struct
{
    const char* comment;
    HGint32     codeOffset;
} sgComment;

/** Return value structure for sgStitch_emitCode */
typedef struct
{
    /** Output size of the program or SG_STITCH_MAX_SIZE_EXCEEDED if
        maximum output size was exceeded. */
    HGint32 outputSize;
    /** Number of output comments generated or zero maximum output
        size was exceeded or no comments could be generated. */
    HGint32 nOutputComments;
} sgReturn;

/*-------------------------------------------------------------------*//*!
 * \brief   Interpret a stitcher program
 * \param   dst             Output pointer
 * \param   maxDestSize     Maximum size of output
 * \param   sticherProgram  Input bytecode stitcher program
 * \param   state           State for bytecode interpretation
 * \param   commentStrings  Comment strings associated with the input 
 *                          program (can be NULL)
 * \param   outputComments  A buffer to store dynamic comment info 
 *                          (if NULL, no output is stored)
 * \param   maxComments     Maximum number of output comments
 *//*-------------------------------------------------------------------*/
HG_EXTERN_C 
sgReturn 
sgInterpretStitcher(void* dst, 
                    int   maxDestSize,
                    const void* sticherProgram, 
                    const void* stitchTimeArgs,
                    const void* stitchConstArgs,
                    const char* const* commentStrings,
                    sgComment* outputComments,
                    int maxComments);

/*-------------------------------------------------------------------*//*!
 * \brief   Interpret a stitcher program
 * \param   dst             Output pointer
 * \param   maxDestSize     Maximum size of output
 * \param   sticherProgram  Input bytecode stitcher program
 * \param   state           State for bytecode interpretation
 * \param   commentStrings  Comment strings associated with the input 
 *                          program (can be NULL)
 * \param   outputComments  A buffer to store dynamic comment info 
 *                          (if NULL, no output is stored)
 * \param   maxComments     Maximum number of output comments

 * \param   tempMem         Temporary memory buffer to be used during
 *                          interpretation.  Needs to hold 
 *                          sgInterpStitchTempMemSize() bytes.
 *//*-------------------------------------------------------------------*/
sgReturn
sgInterpretStitcherExtMem(void* dst,
                          int   maxDestSize,
                          const void* inputProgram,
                          const void* stitchTimeArgs,
                          const void* stitchConstArgs,
                          const char* const* commentStrings,
                          sgComment* outputComments,
                          int maxOutputComments,
                          void* tempMem);

HG_EXTERN_C size_t sgInterpTempMemSize(void);

HG_EXTERN_C
void sgPackSignature(void* dst, 
                     const void* stitchTimeArgs,
                     const void* const compactString);

HG_EXTERN_C
void sgUnpackSignature(void* dst, 
                       const void* packedArgs,
                       const void* const compactString);

#endif /* __SGSTITCHINTERP_H */
