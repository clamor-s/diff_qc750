/*
 *----------------------------------------------------------------
 * Trusted Logic S.A. is an authorized ARM licensee for the use, copy
 * and distribution of this confidential and proprietary software.
 * Notwithstanding the copyright notice below, this confidential and 
 * proprietary software may be used only as authorised by a licensing 
 * agreement from TRUSTED LOGIC S.A. or its distributors with a valid 
 * license agreement for this software.
 * 
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from TRUSTED LOGIC S.A. or its distributors 
 * with a valid license agreement for this software.
 * ----------------------------------------------------------------
 *
 * ----------------------------------------------------------------
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 *   (C) COPYRIGHT 2005-2008 ARM Limited
 *       ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 * ----------------------------------------------------------------
 */

/*
 * File            : smapi_ex.h
 * Last-Author     : Trusted Logic S.A.
 * Created         : April 23, 2007
 */

#ifndef __SMAPI_EX_H__
#define __SMAPI_EX_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Appends the specified UUID value to the encoded data of the specified
 * encoder instance.
 *
 * Upon error, this function sets the error state of the encoder. No other state
 * is affected.
 *
 * This function does nothing if the error state of the encoder is set upon
 * entry.
 *
 * @param hEncoder The handle of the encoder instance.
 *
 * @param pUUID A pointer to the UUID value to encode.
 */
SM_EXPORT void SMStubEncoderWriteUUID(
      SM_HANDLE hEncoder,
      const SM_UUID *pUUID);

/**
 * Reads the UUID value at the current offset in the encoded data parsed by the
 * specified decoder instance.
 *
 * Upon return, the current offset of the decoder references the first item
 * following the decoded item.
 *
 * If the decoder error state is set upon entry, the function sets the UUID
 * placeholder to the nil UUID and does nothing more.
 *
 * @param hDecoder The handle of the decoder instance.
 *
 * @param pUUID A pointer to the placeholder to be set to the decoded UUID
 * value. This placeholder is set to the nil UUID upon failure.
 */
SM_EXPORT void SMStubDecoderReadUUID(
      SM_HANDLE hDecoder,
      SM_UUID *pUUID);

/**
 * Opens a sequence in the current decoder. The current decoder must point to a
 * sequence.
 *
 * After this function is called, the current decoder points to the first element
 * of the sequence.
 *
 * If the error state of the decoder is set upon entry, this function does nothing.
 *
 * Upon error, this function sets the error state of the current decoder.
 * In particular, if the decoder does not point to a sequence, the error state
 * is set to S_ERROR_BAD_FORMAT.
 *
 * @param hDecoder A handle of the decoder instance.
 *
 */
SM_EXPORT void SMStubDecoderOpenSequence(
      SM_HANDLE hDecoder);

/**
 * Closes a sequence in the current decoder. At least one sequence must have been
 * opened using the function {SMStubDecoderOpenSequence}
 *
 * When this function returns, the current decoder points to the first element
 * following the current sequence.
 *
 * If the error state of the decoder is set upon entry, this function does nothing.
 *
 * Upon error, this function sets the error state of the current decoder.
 * In particular, if the decoder does not point within a sequence, the error state
 * is set to S_ERROR_ILLEGAL_STATE.
 *
 * @param hDecoder A handle of the decoder instance.
 *
 */
SM_EXPORT void SMStubDecoderCloseSequence(
      SM_HANDLE hDecoder);

#ifdef __cplusplus
}
#endif

#endif /* __SMAPI_EX_H__ */
