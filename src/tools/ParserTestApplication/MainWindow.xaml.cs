using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using VtStateMachine;

namespace ParserTestApplication
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window, ITerminalDispatch
    {
        VtStateMachine.Parser p;
        public MainWindow()
        {
            InitializeComponent();
            p = new VtStateMachine.Parser(this);
        }

        private void WindowsXamlHost_XamlRootChanged(object sender, EventArgs e)
        {
            Windows.UI.Xaml.Controls.Button button = (Windows.UI.Xaml.Controls.Button)windowXamlHost.Child;
            if (button != null)
            {
                button.Content = "XAML Button";
                button.Click += Button_Click;
            }
        }

        private void Button_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            string label = (sender as Windows.UI.Xaml.Controls.Button).Content as string;
            // TerminalControl.Class foo = new TerminalControl.Class();

            // textBlock.Text = $"{label} - {DateTime.Now.ToLongTimeString()} - {foo.DoTheThing()}";
            textBlock.Text = $"{label} - {DateTime.Now.ToLongTimeString()}";

            // var b = foo.DoTheThing();
        }


        private void EscButton_Click(object sender, RoutedEventArgs e)
        {
            var txt = InputBox.Text;
            OutBlock.Text = "";
            p.ProcessString("\x1b" + txt);
        }

        private void StringButton_Click(object sender, RoutedEventArgs e)
        {
            var txt = InputBox.Text;
            OutBlock.Text = "";
            p.ProcessString(txt);
        }

        // Terminal Interface -------------------------------------------------
        public bool PrintString(string str)
        {
            OutBlock.Text += $"PrintString({str})";
            return true;
        }

        public bool ExecuteChar(char c)
        {
            OutBlock.Text += $"ExecuteChar({c})";
            return true;
        }

        public bool CursorUp(uint distance)
        {
            OutBlock.Text += $"CursorUp({distance})";
            return true;
        }

        public bool CursorDown(uint distance)
        {
            OutBlock.Text += $"CursorDown({distance})";
            return true;
        }

        public bool CursorForward(uint distance)
        {
            OutBlock.Text += $"CursorForward({distance})";
            return true;
        }

        public bool CursorBackward(uint distance)
        {
            OutBlock.Text += $"CursorBackward({distance})";
            return true;
        }

        public bool CursorPosition(uint row, uint col)
        {
            OutBlock.Text += $"CursorPosition({row}, {col})";
            return true;
        }

        public bool CursorSavePosition()
        {
            OutBlock.Text += $"CursorSavePosition()";
            return true;
        }

        public bool CursorRestorePosition()
        {
            OutBlock.Text += $"CursorRestorePosition()";
            return true;
        }

        public bool CursorVisibility(bool isVisible)
        {
            OutBlock.Text += $"CursorVisibility({isVisible})";
            return true;
        }

        public bool InsertCharacter(uint count)
        {
            OutBlock.Text += $"InsertCharacter({count})";
            return true;
        }

        public bool DeleteCharacter(uint count)
        {
            OutBlock.Text += $"DeleteCharacter({count})";
            return true;
        }

        public bool ScrollUp(uint distance)
        {
            OutBlock.Text += $"ScrollUp({distance})";
            return true;
        }

        public bool ScrollDown(uint distance)
        {
            OutBlock.Text += $"ScrollDown({distance})";
            return true;
        }

        public bool InsertLine(uint lines)
        {
            OutBlock.Text += $"InsertLine({lines})";
            return true;
        }

        public bool DeleteLine(uint lines)
        {
            OutBlock.Text += $"DeleteLine({lines})";
            return true;
        }

        public bool EnableCursorBlinking(bool isBlinking)
        {
            OutBlock.Text += $"EnableCursorBlinking({isBlinking})";
            return true;
        }

        public bool ReverseLineFeed()
        {
            OutBlock.Text = $"ReverseLineFeed()";
            return true;
        }

        public bool SetWindowTitle(string title)
        {
            OutBlock.Text += $"SetWindowTitle({title})";
            return true;
        }

        public bool EraseInDisplay(EraseType eraseType)
        {
            OutBlock.Text += $"EraseInDisplay({eraseType})";
            return true;
        }

        public bool EraseInLine(EraseType eraseType)
        {
            OutBlock.Text += $"EraseInLine({eraseType})";
            return true;
        }

        public bool EraseCharacters(uint count)
        {
            OutBlock.Text += $"EraseCharacters({count})";
            return true;
        }

        public bool SetGraphicsRendition(GraphicsOptions[] opts)
        {
            var flat = string.Join(", ", opts);
            OutBlock.Text += $"SetGraphicsRendition([{flat}])";
            return true;
        }

        public bool SetPrivateModes(PrivateModeParams[] @params)
        {
            var flat = string.Join(", ", @params);
            OutBlock.Text += $"SetPrivateModes([{flat}])";
            return true;
        }

        public bool ResetPrivateModes(PrivateModeParams[] @params)
        {
            var flat = string.Join(", ", @params);
            OutBlock.Text += $"ResetPrivateModes([{flat}])";
            return true;
        }

        public bool RefreshWindow()
        {
            OutBlock.Text += $"RefreshWindow()";
            return true;
        }

        public bool ResizeWindow(uint rows, uint cols)
        {
            OutBlock.Text += $"ResizeWindow({rows}, {cols})";
            return true;
        }

        public bool CursorNextLine(uint distance)
        {
            OutBlock.Text += $"CursorNextLine({distance})";
            return true;
        }

        public bool CursorPrevLine(uint distance)
        {
            OutBlock.Text += $"CursorPrevLine({distance})";
            return true;
        }

        public bool CursorHorizontalPositionAbsolute(uint column)
        {
            OutBlock.Text += $"CursorHorizontalPositionAbsolute({column})";
            return true;
        }

        public bool VerticalLinePositionAbsolute(uint row)
        {
            OutBlock.Text += $"VerticalLinePositionAbsolute({row})";
            return true;
        }

        public bool SetColumns(uint columns)
        {
            OutBlock.Text += $"SetColumns({columns})";
            return true;
        }

        public bool SetCursorKeysMode(bool applicationMode)
        {
            OutBlock.Text += $"SetCursorKeysMode({applicationMode})";
            return true;
        }

        public bool SetKeypadMode(bool applicationMode)
        {
            OutBlock.Text += $"SetKeypadMode({applicationMode})";
            return true;
        }

        public bool SetTopBottomScrollingMargins(uint top, uint bottom)
        {
            OutBlock.Text += $"SetTopBottomScrollingMargins({top}, {bottom})";
            return true;
        }

        public bool UseAlternateScreenBuffer()
        {
            OutBlock.Text += $"UseAlternateScreenBuffer()";
            return true;
        }

        public bool UseMainScreenBuffer()
        {
            OutBlock.Text += $"UseMainScreenBuffer()";
            return true;
        }

        public bool HorizontalTabSet()
        {
            OutBlock.Text += $"HorizontalTabSet()";
            return true;
        }

        public bool ForwardTab(uint numTabs)
        {
            OutBlock.Text += $"ForwardTab({numTabs})";
            return true;
        }

        public bool BackwardsTab(uint numTabs)
        {
            OutBlock.Text += $"BackwardsTab({numTabs})";
            return true;
        }

        public bool TabClear(TabClearType clearType)
        {
            OutBlock.Text += $"TabClear({clearType})";
            return true;
        }

        public bool EnableVT200MouseMode(bool enabled)
        {
            OutBlock.Text += $"EnableVT200MouseMode({enabled})";
            return true;
        }

        public bool EnableUTF8ExtendedMouseMode(bool enabled)
        {
            OutBlock.Text += $"EnableUTF8ExtendedMouseMode({enabled})";
            return true;
        }

        public bool EnableSGRExtendedMouseMode(bool enabled)
        {
            OutBlock.Text += $"EnableSGRExtendedMouseMode({enabled})";
            return true;
        }

        public bool EnableButtonEventMouseMode(bool enabled)
        {
            OutBlock.Text += $"EnableButtonEventMouseMode({enabled})";
            return true;
        }

        public bool EnableAnyEventMouseMode(bool enabled)
        {
            OutBlock.Text += $"EnableAnyEventMouseMode({enabled})";
            return true;
        }

        public bool EnableAlternateScroll(bool enabled)
        {
            OutBlock.Text += $"SetCursorStyle({enabled})";
            return true;
        }

        public bool SetColorTableEntry(uint index, uint colorref)
        {
            OutBlock.Text += $"SetCursorStyle({index}, {colorref})";
            return true;
        }

        public bool DeviceStatusReport(AnsiStatusType statusType)
        {
            OutBlock.Text += $"SetCursorStyle({statusType})";
            return true;
        }

        public bool DeviceAttributes()
        {
            OutBlock.Text += $"SetCursorStyle()";
            return true;
        }

        public bool DesignateCharset(CharacterSet charset)
        {
            OutBlock.Text += $"SetCursorStyle({charset})";
            return true;
        }

        public bool SoftReset()
        {
            OutBlock.Text += $"SoftReset()";
            return true;
        }

        public bool HardReset()
        {
            OutBlock.Text += $"HardReset()";
            return true;
        }

        public bool SetCursorStyle(CursorStyle style)
        {
            OutBlock.Text += $"SetCursorStyle({style})";
            return true;
        }

        public bool SetCursorColor(uint colorref)
        {
            OutBlock.Text += $"SetCursorColor({colorref})";
            return true;
        }
    }
}
