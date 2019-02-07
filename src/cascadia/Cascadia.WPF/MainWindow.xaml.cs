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

using TerminalControl;

namespace Cascadia.WPF
{
    enum ShortcutAction
    {
        CopyText,
        PasteText,
        NewTab,
        NewWindow,
        CloseWindow,
        CloseTab,
        SwitchToTab,
        NextTab,
        PrevTab,
        IncreaseFontSize,
        DecreaseFontSize
    }

    public delegate bool NewTabEvent(object sender);
    public delegate bool CloseTabEvent(object sender);
    public delegate bool NewWindowEvent(object sender);
    public delegate bool CloseWindowEvent(object sender);
    public delegate bool CopyEvent(object sender);
    public delegate bool PasteEvent(object sender);

    internal class AppKeyBindings : IKeyBindings
    {
        private Dictionary<ShortcutAction, KeyChord> keyShortcuts;
        public event NewTabEvent NewTab;
        public event CloseTabEvent CloseTab;
        public event NewWindowEvent NewWindow;
        public event CloseWindowEvent CloseWindow;
        public event CopyEvent Copy;
        public event PasteEvent Paste;

        public AppKeyBindings()
        {
            keyShortcuts = new Dictionary<ShortcutAction, KeyChord>();
        }

        public void SetKeyBinding(ShortcutAction action, KeyChord chord)
        {
            keyShortcuts[action] = chord;
        }

        public bool TryKeyChord(KeyChord kc)
        {
            foreach (ShortcutAction action in keyShortcuts.Keys)
            {
                KeyChord binding = keyShortcuts[action];
                if ((binding != null) && (binding.Modifiers == kc.Modifiers && binding.Vkey == kc.Vkey))
                {
                    bool handled = _DoAction(action);
                    if (handled) return handled;
                }
            }
            return false;
        }

        private bool _DoAction(ShortcutAction action)
        {
            switch (action)
            {
                case ShortcutAction.CopyText:
                    Copy(this);
                    break;
                case ShortcutAction.PasteText:
                    Paste(this);
                    break;
                case ShortcutAction.NewTab:
                    NewTab(this);
                    break;
                case ShortcutAction.NewWindow:
                    NewWindow(this);
                    break;
                case ShortcutAction.CloseWindow:
                    CloseWindow(this);
                    break;
                case ShortcutAction.CloseTab:
                    CloseTab(this);
                    break;
            }
            return false;
        }
    }

    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        TermControl term;

        public MainWindow()
        {
            InitializeComponent();
        }

        private void WindowsXamlHost_XamlRootChanged0(object sender, EventArgs args)
        {
            Windows.UI.Xaml.Controls.Grid grid = (Windows.UI.Xaml.Controls.Grid)terminalHost.Child;
            if (grid != null)
            {
                // ARGB is 0xAABBGGRR, don't forget
                TerminalSettings settings = new TerminalSettings();
                settings.DefaultBackground = 0xff008a;
                settings.UseAcrylic = true;
                settings.TintOpacity = 0.5;
                settings.FontSize = 14;
                settings.FontFace = "Ubuntu Mono";
                // DON'T SET THIS TO A FONT THAT ISN'T INSTALLED
                // settings.FontFace = "Foo";

                AppKeyBindings bindings = new AppKeyBindings();
                bindings.SetKeyBinding(ShortcutAction.NewTab, new KeyChord(KeyModifiers.Ctrl, (int)'T'));
                bindings.NewTab += _DoNewTab;

                settings.KeyBindings = bindings;

                term = new TermControl(settings);
                grid.Children.Add(term.GetControl());

            }
        }

        private bool _DoNewTab(object sender)
        {
            // TODO
            int a = 0;
            a++;
            return true;
        }
    }
}
