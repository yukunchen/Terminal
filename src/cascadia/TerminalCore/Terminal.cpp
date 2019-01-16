#include "precomp.h"
#include "Terminal.hpp"
#include "../../terminal/parser/OutputStateMachineEngine.hpp"
#include "TerminalDispatch.hpp"

using namespace Microsoft::Terminal::Core;
using namespace Microsoft::Console::Types;
using namespace Microsoft::Console::VirtualTerminal;
using namespace Microsoft::Console::Render;

COLORREF ARGB(BYTE a, BYTE r, BYTE g, BYTE b)
{
    return (a<<24) | (b<<16) | (g<<8) | (r);
}
#undef RGB
#define RGB(r, g, b) (ARGB(255, (r), (g), (b)))

std::wstring _KeyEventsToText(std::deque<std::unique_ptr<IInputEvent>>& inEventsToWrite)
{
    std::wstring wstr = L"";
    for(auto& ev : inEventsToWrite)
    {
        if (ev->EventType() == InputEventType::KeyEvent)
        {
            auto& k = static_cast<KeyEvent&>(*ev);
            auto wch = k.GetCharData();
            wstr += wch;
        }
    }
    return wstr;
}

Terminal::Terminal() :
    _visibleViewport{Viewport::Empty()},
    _title{ L"" },
    _fontInfo{ L"Consolas", 0, 0, {8, 12}, 95001, false },
    _colorTable{},
    _defaultFg{ RGB(255, 255, 255) },
    _defaultBg{ ARGB(0, 0, 0, 0) },
    _pfnWriteInput{ nullptr }
{
    _stateMachine = std::make_unique<StateMachine>(new OutputStateMachineEngine(new TerminalDispatch(*this)));

    auto passAlongInput = [&](std::deque<std::unique_ptr<IInputEvent>>& inEventsToWrite)
    {
        if(!_pfnWriteInput) return;
        std::wstring wstr = _KeyEventsToText(inEventsToWrite);
        _pfnWriteInput(wstr);
    };

    _terminalInput = std::make_unique<TerminalInput>(passAlongInput);

    _InitializeColorTable();
}

void Terminal::Create(COORD viewportSize, SHORT scrollbackLines, IRenderTarget& renderTarget)
{
    _visibleViewport = Viewport::FromDimensions({ 0,0 }, viewportSize);
    COORD bufferSize { viewportSize.X, viewportSize.Y + scrollbackLines };
    TextAttribute attr{};
    UINT cursorSize = 12;
    _buffer = std::make_unique<TextBuffer>(bufferSize, attr, cursorSize, renderTarget);
}

Viewport Terminal::_GetMutableViewport()
{
    return _visibleViewport;
}

void Terminal::Write(std::wstring_view stringView)
{
    LockForWriting();
    auto a = wil::scope_exit([&]{ UnlockForWriting(); });

    _stateMachine->ProcessString(stringView.data(), stringView.size());
}

bool Terminal::SendKeyEvent(const WORD vkey,
                            const bool ctrlPressed,
                            const bool altPressed,
                            const bool shiftPressed)
{
    DWORD modifiers = 0
                      | (ctrlPressed? LEFT_CTRL_PRESSED : 0)
                      | (altPressed? LEFT_ALT_PRESSED : 0)
                      | (shiftPressed? SHIFT_PRESSED : 0)
                      ;
    KeyEvent keyEv{ true, 0, vkey, 0, L'\0', modifiers};
    return _terminalInput->HandleKey(&keyEv);
}


void Terminal::LockForReading()
{
    _readWriteLock.lock_shared();
}
void Terminal::LockForWriting()
{
    _readWriteLock.lock();
}
void Terminal::UnlockForReading()
{
    _readWriteLock.unlock_shared();
}
void Terminal::UnlockForWriting()
{
    _readWriteLock.unlock();
}

void Terminal::_InitializeColorTable()
{
    _colorTable[0]   = RGB( 12,   12,   12);
    _colorTable[1]   = RGB( 197,  15,   31);
    _colorTable[2]   = RGB( 19,   161,  14);
    _colorTable[3]   = RGB( 193,  156,  0);
    _colorTable[4]   = RGB( 0,    55,   218);
    _colorTable[5]   = RGB( 136,  23,   152);
    _colorTable[6]   = RGB( 58,   150,  221);
    _colorTable[7]   = RGB( 204,  204,  204);
    _colorTable[8]   = RGB( 118,  118,  118);
    _colorTable[9]   = RGB( 231,  72,   86);
    _colorTable[10]  = RGB( 22,   198,  12);
    _colorTable[11]  = RGB( 249,  241,  165);
    _colorTable[12]  = RGB( 59,   120,  255);
    _colorTable[13]  = RGB( 180,  0,    158);
    _colorTable[14]  = RGB( 97,   214,  214);
    _colorTable[15]  = RGB( 242,  242,  242);
    _colorTable[16]  = RGB( 0x00, 0x00, 0x00);
    _colorTable[17]  = RGB( 0x00, 0x00, 0x5f);
    _colorTable[18]  = RGB( 0x00, 0x00, 0x87);
    _colorTable[19]  = RGB( 0x00, 0x00, 0xaf);
    _colorTable[20]  = RGB( 0x00, 0x00, 0xd7);
    _colorTable[21]  = RGB( 0x00, 0x00, 0xff);
    _colorTable[22]  = RGB( 0x00, 0x5f, 0x00);
    _colorTable[23]  = RGB( 0x00, 0x5f, 0x5f);
    _colorTable[24]  = RGB( 0x00, 0x5f, 0x87);
    _colorTable[25]  = RGB( 0x00, 0x5f, 0xaf);
    _colorTable[26]  = RGB( 0x00, 0x5f, 0xd7);
    _colorTable[27]  = RGB( 0x00, 0x5f, 0xff);
    _colorTable[28]  = RGB( 0x00, 0x87, 0x00);
    _colorTable[29]  = RGB( 0x00, 0x87, 0x5f);
    _colorTable[30]  = RGB( 0x00, 0x87, 0x87);
    _colorTable[31]  = RGB( 0x00, 0x87, 0xaf);
    _colorTable[32]  = RGB( 0x00, 0x87, 0xd7);
    _colorTable[33]  = RGB( 0x00, 0x87, 0xff);
    _colorTable[34]  = RGB( 0x00, 0xaf, 0x00);
    _colorTable[35]  = RGB( 0x00, 0xaf, 0x5f);
    _colorTable[36]  = RGB( 0x00, 0xaf, 0x87);
    _colorTable[37]  = RGB( 0x00, 0xaf, 0xaf);
    _colorTable[38]  = RGB( 0x00, 0xaf, 0xd7);
    _colorTable[39]  = RGB( 0x00, 0xaf, 0xff);
    _colorTable[40]  = RGB( 0x00, 0xd7, 0x00);
    _colorTable[41]  = RGB( 0x00, 0xd7, 0x5f);
    _colorTable[42]  = RGB( 0x00, 0xd7, 0x87);
    _colorTable[43]  = RGB( 0x00, 0xd7, 0xaf);
    _colorTable[44]  = RGB( 0x00, 0xd7, 0xd7);
    _colorTable[45]  = RGB( 0x00, 0xd7, 0xff);
    _colorTable[46]  = RGB( 0x00, 0xff, 0x00);
    _colorTable[47]  = RGB( 0x00, 0xff, 0x5f);
    _colorTable[48]  = RGB( 0x00, 0xff, 0x87);
    _colorTable[49]  = RGB( 0x00, 0xff, 0xaf);
    _colorTable[50]  = RGB( 0x00, 0xff, 0xd7);
    _colorTable[51]  = RGB( 0x00, 0xff, 0xff);
    _colorTable[52]  = RGB( 0x5f, 0x00, 0x00);
    _colorTable[53]  = RGB( 0x5f, 0x00, 0x5f);
    _colorTable[54]  = RGB( 0x5f, 0x00, 0x87);
    _colorTable[55]  = RGB( 0x5f, 0x00, 0xaf);
    _colorTable[56]  = RGB( 0x5f, 0x00, 0xd7);
    _colorTable[57]  = RGB( 0x5f, 0x00, 0xff);
    _colorTable[58]  = RGB( 0x5f, 0x5f, 0x00);
    _colorTable[59]  = RGB( 0x5f, 0x5f, 0x5f);
    _colorTable[60]  = RGB( 0x5f, 0x5f, 0x87);
    _colorTable[61]  = RGB( 0x5f, 0x5f, 0xaf);
    _colorTable[62]  = RGB( 0x5f, 0x5f, 0xd7);
    _colorTable[63]  = RGB( 0x5f, 0x5f, 0xff);
    _colorTable[64]  = RGB( 0x5f, 0x87, 0x00);
    _colorTable[65]  = RGB( 0x5f, 0x87, 0x5f);
    _colorTable[66]  = RGB( 0x5f, 0x87, 0x87);
    _colorTable[67]  = RGB( 0x5f, 0x87, 0xaf);
    _colorTable[68]  = RGB( 0x5f, 0x87, 0xd7);
    _colorTable[69]  = RGB( 0x5f, 0x87, 0xff);
    _colorTable[70]  = RGB( 0x5f, 0xaf, 0x00);
    _colorTable[71]  = RGB( 0x5f, 0xaf, 0x5f);
    _colorTable[72]  = RGB( 0x5f, 0xaf, 0x87);
    _colorTable[73]  = RGB( 0x5f, 0xaf, 0xaf);
    _colorTable[74]  = RGB( 0x5f, 0xaf, 0xd7);
    _colorTable[75]  = RGB( 0x5f, 0xaf, 0xff);
    _colorTable[76]  = RGB( 0x5f, 0xd7, 0x00);
    _colorTable[77]  = RGB( 0x5f, 0xd7, 0x5f);
    _colorTable[78]  = RGB( 0x5f, 0xd7, 0x87);
    _colorTable[79]  = RGB( 0x5f, 0xd7, 0xaf);
    _colorTable[80]  = RGB( 0x5f, 0xd7, 0xd7);
    _colorTable[81]  = RGB( 0x5f, 0xd7, 0xff);
    _colorTable[82]  = RGB( 0x5f, 0xff, 0x00);
    _colorTable[83]  = RGB( 0x5f, 0xff, 0x5f);
    _colorTable[84]  = RGB( 0x5f, 0xff, 0x87);
    _colorTable[85]  = RGB( 0x5f, 0xff, 0xaf);
    _colorTable[86]  = RGB( 0x5f, 0xff, 0xd7);
    _colorTable[87]  = RGB( 0x5f, 0xff, 0xff);
    _colorTable[88]  = RGB( 0x87, 0x00, 0x00);
    _colorTable[89]  = RGB( 0x87, 0x00, 0x5f);
    _colorTable[90]  = RGB( 0x87, 0x00, 0x87);
    _colorTable[91]  = RGB( 0x87, 0x00, 0xaf);
    _colorTable[92]  = RGB( 0x87, 0x00, 0xd7);
    _colorTable[93]  = RGB( 0x87, 0x00, 0xff);
    _colorTable[94]  = RGB( 0x87, 0x5f, 0x00);
    _colorTable[95]  = RGB( 0x87, 0x5f, 0x5f);
    _colorTable[96]  = RGB( 0x87, 0x5f, 0x87);
    _colorTable[97]  = RGB( 0x87, 0x5f, 0xaf);
    _colorTable[98]  = RGB( 0x87, 0x5f, 0xd7);
    _colorTable[99]  = RGB( 0x87, 0x5f, 0xff);
    _colorTable[100] = RGB(0x87, 0x87, 0x00);
    _colorTable[101] = RGB(0x87, 0x87, 0x5f);
    _colorTable[102] = RGB(0x87, 0x87, 0x87);
    _colorTable[103] = RGB(0x87, 0x87, 0xaf);
    _colorTable[104] = RGB(0x87, 0x87, 0xd7);
    _colorTable[105] = RGB(0x87, 0x87, 0xff);
    _colorTable[106] = RGB(0x87, 0xaf, 0x00);
    _colorTable[107] = RGB(0x87, 0xaf, 0x5f);
    _colorTable[108] = RGB(0x87, 0xaf, 0x87);
    _colorTable[109] = RGB(0x87, 0xaf, 0xaf);
    _colorTable[110] = RGB(0x87, 0xaf, 0xd7);
    _colorTable[111] = RGB(0x87, 0xaf, 0xff);
    _colorTable[112] = RGB(0x87, 0xd7, 0x00);
    _colorTable[113] = RGB(0x87, 0xd7, 0x5f);
    _colorTable[114] = RGB(0x87, 0xd7, 0x87);
    _colorTable[115] = RGB(0x87, 0xd7, 0xaf);
    _colorTable[116] = RGB(0x87, 0xd7, 0xd7);
    _colorTable[117] = RGB(0x87, 0xd7, 0xff);
    _colorTable[118] = RGB(0x87, 0xff, 0x00);
    _colorTable[119] = RGB(0x87, 0xff, 0x5f);
    _colorTable[120] = RGB(0x87, 0xff, 0x87);
    _colorTable[121] = RGB(0x87, 0xff, 0xaf);
    _colorTable[122] = RGB(0x87, 0xff, 0xd7);
    _colorTable[123] = RGB(0x87, 0xff, 0xff);
    _colorTable[124] = RGB(0xaf, 0x00, 0x00);
    _colorTable[125] = RGB(0xaf, 0x00, 0x5f);
    _colorTable[126] = RGB(0xaf, 0x00, 0x87);
    _colorTable[127] = RGB(0xaf, 0x00, 0xaf);
    _colorTable[128] = RGB(0xaf, 0x00, 0xd7);
    _colorTable[129] = RGB(0xaf, 0x00, 0xff);
    _colorTable[130] = RGB(0xaf, 0x5f, 0x00);
    _colorTable[131] = RGB(0xaf, 0x5f, 0x5f);
    _colorTable[132] = RGB(0xaf, 0x5f, 0x87);
    _colorTable[133] = RGB(0xaf, 0x5f, 0xaf);
    _colorTable[134] = RGB(0xaf, 0x5f, 0xd7);
    _colorTable[135] = RGB(0xaf, 0x5f, 0xff);
    _colorTable[136] = RGB(0xaf, 0x87, 0x00);
    _colorTable[137] = RGB(0xaf, 0x87, 0x5f);
    _colorTable[138] = RGB(0xaf, 0x87, 0x87);
    _colorTable[139] = RGB(0xaf, 0x87, 0xaf);
    _colorTable[140] = RGB(0xaf, 0x87, 0xd7);
    _colorTable[141] = RGB(0xaf, 0x87, 0xff);
    _colorTable[142] = RGB(0xaf, 0xaf, 0x00);
    _colorTable[143] = RGB(0xaf, 0xaf, 0x5f);
    _colorTable[144] = RGB(0xaf, 0xaf, 0x87);
    _colorTable[145] = RGB(0xaf, 0xaf, 0xaf);
    _colorTable[146] = RGB(0xaf, 0xaf, 0xd7);
    _colorTable[147] = RGB(0xaf, 0xaf, 0xff);
    _colorTable[148] = RGB(0xaf, 0xd7, 0x00);
    _colorTable[149] = RGB(0xaf, 0xd7, 0x5f);
    _colorTable[150] = RGB(0xaf, 0xd7, 0x87);
    _colorTable[151] = RGB(0xaf, 0xd7, 0xaf);
    _colorTable[152] = RGB(0xaf, 0xd7, 0xd7);
    _colorTable[153] = RGB(0xaf, 0xd7, 0xff);
    _colorTable[154] = RGB(0xaf, 0xff, 0x00);
    _colorTable[155] = RGB(0xaf, 0xff, 0x5f);
    _colorTable[156] = RGB(0xaf, 0xff, 0x87);
    _colorTable[157] = RGB(0xaf, 0xff, 0xaf);
    _colorTable[158] = RGB(0xaf, 0xff, 0xd7);
    _colorTable[159] = RGB(0xaf, 0xff, 0xff);
    _colorTable[160] = RGB(0xd7, 0x00, 0x00);
    _colorTable[161] = RGB(0xd7, 0x00, 0x5f);
    _colorTable[162] = RGB(0xd7, 0x00, 0x87);
    _colorTable[163] = RGB(0xd7, 0x00, 0xaf);
    _colorTable[164] = RGB(0xd7, 0x00, 0xd7);
    _colorTable[165] = RGB(0xd7, 0x00, 0xff);
    _colorTable[166] = RGB(0xd7, 0x5f, 0x00);
    _colorTable[167] = RGB(0xd7, 0x5f, 0x5f);
    _colorTable[168] = RGB(0xd7, 0x5f, 0x87);
    _colorTable[169] = RGB(0xd7, 0x5f, 0xaf);
    _colorTable[170] = RGB(0xd7, 0x5f, 0xd7);
    _colorTable[171] = RGB(0xd7, 0x5f, 0xff);
    _colorTable[172] = RGB(0xd7, 0x87, 0x00);
    _colorTable[173] = RGB(0xd7, 0x87, 0x5f);
    _colorTable[174] = RGB(0xd7, 0x87, 0x87);
    _colorTable[175] = RGB(0xd7, 0x87, 0xaf);
    _colorTable[176] = RGB(0xd7, 0x87, 0xd7);
    _colorTable[177] = RGB(0xd7, 0x87, 0xff);
    _colorTable[178] = RGB(0xdf, 0xaf, 0x00);
    _colorTable[179] = RGB(0xdf, 0xaf, 0x5f);
    _colorTable[180] = RGB(0xdf, 0xaf, 0x87);
    _colorTable[181] = RGB(0xdf, 0xaf, 0xaf);
    _colorTable[182] = RGB(0xdf, 0xaf, 0xd7);
    _colorTable[183] = RGB(0xdf, 0xaf, 0xff);
    _colorTable[184] = RGB(0xdf, 0xd7, 0x00);
    _colorTable[185] = RGB(0xdf, 0xd7, 0x5f);
    _colorTable[186] = RGB(0xdf, 0xd7, 0x87);
    _colorTable[187] = RGB(0xdf, 0xd7, 0xaf);
    _colorTable[188] = RGB(0xdf, 0xd7, 0xd7);
    _colorTable[189] = RGB(0xdf, 0xd7, 0xff);
    _colorTable[190] = RGB(0xdf, 0xff, 0x00);
    _colorTable[191] = RGB(0xdf, 0xff, 0x5f);
    _colorTable[192] = RGB(0xdf, 0xff, 0x87);
    _colorTable[193] = RGB(0xdf, 0xff, 0xaf);
    _colorTable[194] = RGB(0xdf, 0xff, 0xd7);
    _colorTable[195] = RGB(0xdf, 0xff, 0xff);
    _colorTable[196] = RGB(0xff, 0x00, 0x00);
    _colorTable[197] = RGB(0xff, 0x00, 0x5f);
    _colorTable[198] = RGB(0xff, 0x00, 0x87);
    _colorTable[199] = RGB(0xff, 0x00, 0xaf);
    _colorTable[200] = RGB(0xff, 0x00, 0xd7);
    _colorTable[201] = RGB(0xff, 0x00, 0xff);
    _colorTable[202] = RGB(0xff, 0x5f, 0x00);
    _colorTable[203] = RGB(0xff, 0x5f, 0x5f);
    _colorTable[204] = RGB(0xff, 0x5f, 0x87);
    _colorTable[205] = RGB(0xff, 0x5f, 0xaf);
    _colorTable[206] = RGB(0xff, 0x5f, 0xd7);
    _colorTable[207] = RGB(0xff, 0x5f, 0xff);
    _colorTable[208] = RGB(0xff, 0x87, 0x00);
    _colorTable[209] = RGB(0xff, 0x87, 0x5f);
    _colorTable[210] = RGB(0xff, 0x87, 0x87);
    _colorTable[211] = RGB(0xff, 0x87, 0xaf);
    _colorTable[212] = RGB(0xff, 0x87, 0xd7);
    _colorTable[213] = RGB(0xff, 0x87, 0xff);
    _colorTable[214] = RGB(0xff, 0xaf, 0x00);
    _colorTable[215] = RGB(0xff, 0xaf, 0x5f);
    _colorTable[216] = RGB(0xff, 0xaf, 0x87);
    _colorTable[217] = RGB(0xff, 0xaf, 0xaf);
    _colorTable[218] = RGB(0xff, 0xaf, 0xd7);
    _colorTable[219] = RGB(0xff, 0xaf, 0xff);
    _colorTable[220] = RGB(0xff, 0xd7, 0x00);
    _colorTable[221] = RGB(0xff, 0xd7, 0x5f);
    _colorTable[222] = RGB(0xff, 0xd7, 0x87);
    _colorTable[223] = RGB(0xff, 0xd7, 0xaf);
    _colorTable[224] = RGB(0xff, 0xd7, 0xd7);
    _colorTable[225] = RGB(0xff, 0xd7, 0xff);
    _colorTable[226] = RGB(0xff, 0xff, 0x00);
    _colorTable[227] = RGB(0xff, 0xff, 0x5f);
    _colorTable[228] = RGB(0xff, 0xff, 0x87);
    _colorTable[229] = RGB(0xff, 0xff, 0xaf);
    _colorTable[230] = RGB(0xff, 0xff, 0xd7);
    _colorTable[231] = RGB(0xff, 0xff, 0xff);
    _colorTable[232] = RGB(0x08, 0x08, 0x08);
    _colorTable[233] = RGB(0x12, 0x12, 0x12);
    _colorTable[234] = RGB(0x1c, 0x1c, 0x1c);
    _colorTable[235] = RGB(0x26, 0x26, 0x26);
    _colorTable[236] = RGB(0x30, 0x30, 0x30);
    _colorTable[237] = RGB(0x3a, 0x3a, 0x3a);
    _colorTable[238] = RGB(0x44, 0x44, 0x44);
    _colorTable[239] = RGB(0x4e, 0x4e, 0x4e);
    _colorTable[240] = RGB(0x58, 0x58, 0x58);
    _colorTable[241] = RGB(0x62, 0x62, 0x62);
    _colorTable[242] = RGB(0x6c, 0x6c, 0x6c);
    _colorTable[243] = RGB(0x76, 0x76, 0x76);
    _colorTable[244] = RGB(0x80, 0x80, 0x80);
    _colorTable[245] = RGB(0x8a, 0x8a, 0x8a);
    _colorTable[246] = RGB(0x94, 0x94, 0x94);
    _colorTable[247] = RGB(0x9e, 0x9e, 0x9e);
    _colorTable[248] = RGB(0xa8, 0xa8, 0xa8);
    _colorTable[249] = RGB(0xb2, 0xb2, 0xb2);
    _colorTable[250] = RGB(0xbc, 0xbc, 0xbc);
    _colorTable[251] = RGB(0xc6, 0xc6, 0xc6);
    _colorTable[252] = RGB(0xd0, 0xd0, 0xd0);
    _colorTable[253] = RGB(0xda, 0xda, 0xda);
    _colorTable[254] = RGB(0xe4, 0xe4, 0xe4);
    _colorTable[255] = RGB(0xee, 0xee, 0xee);
}
