uint64_t __fastcall MmAllocateIndependentPages(SIZE_T number_of_bytes, ULONG node) {
    SIZE_T page_count = BYTES_TO_PAGES(number_of_bytes);
    _MMPTE* pte = MiReservePtes((_MMPTE*)&g_pt_base, (uint32_t)page_count);
    if (!pte) // null if no pte's can be found
        return 0;

    // If no non paged pool memory is available, return 0
    if (!MiObtainNonPagedPoolCharges(page_count, 1)) {
        MiReleasePtes((_MMPTE*)&g_pt_base, pte, page_count);
        return 0;
    }

    uint64_t result = (uint64_t)MiGetVirtualAddressMappedByPte(pte);

    _MMPTE* new_pte = MiMakeValidPte(pte, 0, 0xA0000004); // PTE*, frame, protection mask (4 is READWRITE)

    int32_t* system_page_color;
    MiInitializePageColorBase(0, node + 1, &system_page_color);

    // Create valid pte entry for each page
    do {
        int32_t page_color = (int32_t)(++*(_WORD*)system_page_color);

        // Wait for free page_frame_index
        uint32_t page_frame_index;
        while (true) {
            page_frame_index = MiGetPage((int64_t)&MiSystemPartition, page_color, 8u);
			
            if (page_frame_index != -1)
                break;
				
            MiWaitForFreePage(&MiSystemPartition);
        }

        new_pte = (_MMPTE*)(((uint64_t)new_pte ^ PFN_TO_PAGE(page_frame_index)) & 0xFFFFFFFFF000 ^ (uint64_t)new_pte);

		_MMPFN* pfn_entry = (_MMPFN*)(0xFFFFFa8000000000 + sizeof(_MMPFN) * page_frame_index);
        MiInitializePfn(pfn_entry, pte, 4, 4);

        pte - > u.Long = (uint64_t)new_pte; // set available pte to new, valid pte

        if (MiPteInShadowRange(pte))
            MiWritePteShadow(0, new_pte);

        ++pte;
        --page_count;
    }
    while (page_count);
    return result;
}
