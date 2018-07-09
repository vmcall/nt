unsigned __int64 __fastcall MmAllocateIndependentPages(SIZE_T number_of_bytes, ULONG node)
{
  SIZE_T page_count = (number_of_bytes >> 12) + ((number_of_bytes & 0xFFF) != 0);// BYTES_TO_PAGES macro, number_of_bytes >> 12 == number_of_bytes / 0x1000
  _MMPTE* pte = MiReservePtes((_MMPTE*)&g_pt_base, (unsigned int)page_count);
  if ( !pte )                                   // null if no pte's can be found
    return 0i64;
  // If no non paged pool memory is available, return 0
  if ( !(unsigned int)MiObtainNonPagedPoolCharges(page_count, 1) )
  {
    MiReleasePtes((_MMPTE *)&g_pt_base, pte, page_count);
    return 0i64;
  }
  unsigned __int64 result = (_QWORD)pte << 25 >> 16;             // MiGetVirtualAddressMappedByPte macro
  _MMPTE* new_pte = MiMakeValidPte(pte, 0i64, 0xA0000004);// PTE*, frame, protection mask (4 is READWRITE)
  __int32* system_page_color; 
  MiInitializePageColorBase(0i64, node + 1, &system_page_color);
  __int32* system_page_color_1 = system_page_color;
  // create valid pte entry for each page
  do
  {
	ULONG_PTR page_frame_index;
    int page_color = 0 | (unsigned __int16)(++*(_WORD*)system_page_color_1);
    // Wait for free page_frame_index
    while ( 1 )
    {
      ULONG_PTR page_frame_index = MiGetPage((__int64)&MiSystemPartition, page_color, 8u);
      if ( page_frame_index != -1i64 )
        break;
      MiWaitForFreePage(&MiSystemPartition);
    }
    new_pte = (_MMPTE *)(((unsigned __int64)new_pte ^ (page_frame_index << 12)) & 0xFFFFFFFFF000i64 ^ (unsigned __int64)new_pte);// 'page_frame_index << 12' PFN_TO_PAGE macro
    MiInitializePfn((_MMPFN *)(48 * page_frame_index - 0x58000000000i64), pte, 4i64, 4);// 48 * page_frame_index - 0x58000000000i64 calculates address of pfn database entry
    pte->u.Long = (unsigned __int64)new_pte;    // set available pte to new, valid pte
    if (MiPteInShadowRange(pte))
      MiWritePteShadow(0, new_pte);
    ++pte;
    --page_count;
  }
  while (page_count);
  return result;
}
