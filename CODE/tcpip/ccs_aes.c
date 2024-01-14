#ifndef __AES_PIC24_C__

void AESCreateRoundKeys(void* round_keys, UINT8* key, UINT8 key_size)
{
}

void AESCFBEncrypt(UINT8 * cipher_text, UINT8 * plain_text, UINT32 num_bytes, void * round_keys, AES_CFB_STATE_DATA *p_cfb_state_data, UINT32 options)
{
}

void AESCFBDecrypt(UINT8 * plain_text, UINT8 * cipher_text, UINT32 num_bytes, void * round_keys, AES_CFB_STATE_DATA *p_cfb_state_data, UINT32 options)
{
}

#endif
