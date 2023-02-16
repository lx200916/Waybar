// adapted from https://unicode.org/Public/UNIDATA/EastAsianWidth.txt
bool char_is_wide(uint32_t c) {
    switch (c) {
        case 0x231A:
        case 0x231B:
        case 0x2329:
        case 0x232A:
        case 0x23F0:
        case 0x23F3:
        case 0x25FD:
        case 0x25FE:
        case 0x2614:
        case 0x2615:
        case 0x267F:
        case 0x2693:
        case 0x26A1:
        case 0x26AA:
        case 0x26AB:
        case 0x26BD:
        case 0x26BE:
        case 0x26C4:
        case 0x26C5:
        case 0x26CE:
        case 0x26D4:
        case 0x26EA:
        case 0x26F2:
        case 0x26F3:
        case 0x26F5:
        case 0x26FA:
        case 0x26FD:
        case 0x2705:
        case 0x270A:
        case 0x270B:
        case 0x2728:
        case 0x274C:
        case 0x274E:
        case 0x2753:
        case 0x2754:
        case 0x2755:
        case 0x2757:
        case 0x2795:
        case 0x2796:
        case 0x2797:
        case 0x27B0:
        case 0x27BF:
        case 0x2B1B:
        case 0x2B1C:
        case 0x2B50:
        case 0x2B55:
        case 0x302E:
        case 0x302F:
        case 0x3030:
        case 0x3036:
        case 0x3037:
        case 0x309F:
        case 0x30A0:
        case 0x30FB:
        case 0x30FF:
        case 0x3250:
        case 0xA015:
        case 0xFE17:
        case 0xFE18:
        case 0xFE19:
        case 0xFE30:
        case 0x16FF0:
        case 0x16FF1:
        case 0x1B132:
        case 0x1B150:
        case 0x1B151:
        case 0x1B152:
        case 0x1B155:
        case 0x1F004:
        case 0x1F0CF:
        case 0x1F18E:
        case 0x1F200:
        case 0x1F201:
        case 0x1F202:
        case 0x1F250:
        case 0x1F251:
        case 0x1F3F4:
        case 0x1F440:
        case 0x1F57A:
        case 0x1F595:
        case 0x1F596:
        case 0x1F5A4:
        case 0x1F6CC:
        case 0x1F6D5:
        case 0x1F6D6:
        case 0x1F6D7:
        case 0x1F6EB:
        case 0x1F6EC:
        case 0x1F7F0:
            return true;
        default:
            break;
    }
    if ((c >= 0x1100 && c <= 0x115F)
        || (c >= 0x23E9 && c <= 0x23EC)
        || (c >= 0x2648 && c <= 0x2653)
        || (c >= 0x2E80 && c <= 0x2E99)
        || (c >= 0x2E9B && c <= 0x2EF3)
        || (c >= 0x2F00 && c <= 0x2FD5)
        || (c >= 0x2FF0 && c <= 0x2FFB)
        || (c >= 0x3000 && c <= 0x302D)
        || (c >= 0x3031 && c <= 0x3035)
        || (c >= 0x3038 && c <= 0x303E)
        || (c >= 0x3041 && c <= 0x3096)
        || (c >= 0x3099 && c <= 0x309E)
        || (c >= 0x30A1 && c <= 0x30FA)
        || (c >= 0x30FC && c <= 0x30FE)
        || (c >= 0x3105 && c <= 0x312F)
        || (c >= 0x3131 && c <= 0x318E)
        || (c >= 0x3190 && c <= 0x319F)
        || (c >= 0x31A0 && c <= 0x31BF)
        || (c >= 0x31C0 && c <= 0x31E3)
        || (c >= 0x31F0 && c <= 0x31FF)
        || (c >= 0x3200 && c <= 0x321E)
        || (c >= 0x3220 && c <= 0x3247)
        || (c >= 0x3251 && c <= 0x4DBF)
        || (c >= 0x4E00 && c <= 0xA014)
        || (c >= 0xA016 && c <= 0xA48C)
        || (c >= 0xA490 && c <= 0xA4C6)
        || (c >= 0xA960 && c <= 0xA97C)
        || (c >= 0xAC00 && c <= 0xD7A3)
        || (c >= 0xF900 && c <= 0xFAFF)
        || (c >= 0xFE10 && c <= 0xFE16)
        || (c >= 0xFE31 && c <= 0xFE52)
        || (c >= 0xFE54 && c <= 0xFE66)
        || (c >= 0xFE68 && c <= 0xFE6B)
        || (c >= 0xFF01 && c <= 0xFF60)
        || (c >= 0xFFE0 && c <= 0xFFE6)
        || (c >= 0x16FE0 && c <= 0x16FE4)
        || (c >= 0x17000 && c <= 0x187F7)
        || (c >= 0x18800 && c <= 0x18CD5)
        || (c >= 0x18D00 && c <= 0x18D08)
        || (c >= 0x1AFF0 && c <= 0x1AFF3)
        || (c >= 0x1AFF5 && c <= 0x1AFFB)
        || (c >= 0x1AFFD && c <= 0x1AFFE)
        || (c >= 0x1B000 && c <= 0x1B122)
        || (c >= 0x1B164 && c <= 0x1B167)
        || (c >= 0x1B170 && c <= 0x1B2FB)
        || (c >= 0x1F191 && c <= 0x1F19A)
        || (c >= 0x1F210 && c <= 0x1F23B)
        || (c >= 0x1F240 && c <= 0x1F248)
        || (c >= 0x1F260 && c <= 0x1F265)
        || (c >= 0x1F300 && c <= 0x1F320)
        || (c >= 0x1F32D && c <= 0x1F335)
        || (c >= 0x1F337 && c <= 0x1F37C)
        || (c >= 0x1F37E && c <= 0x1F393)
        || (c >= 0x1F3A0 && c <= 0x1F3CA)
        || (c >= 0x1F3CF && c <= 0x1F3D3)
        || (c >= 0x1F3E0 && c <= 0x1F3F0)
        || (c >= 0x1F3F8 && c <= 0x1F43E)
        || (c >= 0x1F442 && c <= 0x1F4FC)
        || (c >= 0x1F4FF && c <= 0x1F53D)
        || (c >= 0x1F54B && c <= 0x1F54E)
        || (c >= 0x1F550 && c <= 0x1F567)
        || (c >= 0x1F5FB && c <= 0x1F64F)
        || (c >= 0x1F680 && c <= 0x1F6C5)
        || (c >= 0x1F6D0 && c <= 0x1F6D2)
        || (c >= 0x1F6DC && c <= 0x1F6DF)
        || (c >= 0x1F6F4 && c <= 0x1F6FC)
        || (c >= 0x1F7E0 && c <= 0x1F7EB)
        || (c >= 0x1F90C && c <= 0x1F93A)
        || (c >= 0x1F93C && c <= 0x1F945)
        || (c >= 0x1F947 && c <= 0x1F9FF)
        || (c >= 0x1FA70 && c <= 0x1FA7C)
        || (c >= 0x1FA80 && c <= 0x1FA88)
        || (c >= 0x1FA90 && c <= 0x1FABD)
        || (c >= 0x1FABF && c <= 0x1FAC5)
        || (c >= 0x1FACE && c <= 0x1FADB)
        || (c >= 0x1FAE0 && c <= 0x1FAE8)
        || (c >= 0x1FAF0 && c <= 0x1FAF8)
        || (c >= 0x20000 && c <= 0x2FFFD)
        || (c >= 0x30000 && c <= 0x3FFFD))
    {
        return true;
    }
    return false;
}
