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
                    return Copy(this);
                case ShortcutAction.PasteText:
                    return Paste(this);
                case ShortcutAction.NewTab:
                    return NewTab(this);
                case ShortcutAction.NewWindow:
                    return NewWindow(this);
                case ShortcutAction.CloseWindow:
                    return CloseWindow(this);
                case ShortcutAction.CloseTab:
                    return CloseTab(this);
            }
            return false;
        }
    }

}
