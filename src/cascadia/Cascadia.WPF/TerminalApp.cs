using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;

using TerminalControl;
using Windows.UI.Xaml.Controls;

namespace Cascadia.WPF
{
    internal class Tab : IDisposable
    {
        public TermControl terminal;
        public Windows.UI.Xaml.Controls.Button tabButton;
        public bool focused;
        public Tab(TermControl terminal)
        {
            this.tabButton = null;
            this.terminal = terminal;
            this.focused = false;
            _MakeTabButton();
        }

        private void _MakeTabButton()
        {
            Windows.UI.Xaml.Controls.Button button = new Windows.UI.Xaml.Controls.Button();
            button.Content = $"{this.terminal.GetTitle()}";
            this.tabButton = button;
            this.terminal.TitleChanged += (string newTitle) => {
                this.tabButton.Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
                {
                    this.tabButton.Content = $"{newTitle}";
                });
            };
            this.tabButton.FontSize = 12;
        }

        #region IDisposable Support
        private bool disposedValue = false; // To detect redundant calls

        protected virtual void Dispose(bool disposing)
        {
            if (!disposedValue)
            {
                if (disposing)
                {
                    // TODO: dispose managed state (managed objects).
                    terminal.Close();
                }

                // TODO: free unmanaged resources (unmanaged objects) and override a finalizer below.
                // TODO: set large fields to null.
                terminal = null;
                disposedValue = true;
            }
        }

        // TODO: override a finalizer only if Dispose(bool disposing) above has code to free unmanaged resources.
        // ~Tab() {
        //   // Do not change this code. Put cleanup code in Dispose(bool disposing) above.
        //   Dispose(false);
        // }

        // This code added to correctly implement the disposable pattern.
        public void Dispose()
        {
            // Do not change this code. Put cleanup code in Dispose(bool disposing) above.
            Dispose(true);
            // TODO: uncomment the following line if the finalizer is overridden above.
            // GC.SuppressFinalize(this);
        }
        #endregion
    }

    public class TerminalApp
    {
        private List<Tab> tabs;
        private Windows.UI.Xaml.Controls.Grid root;
        private Windows.UI.Xaml.Controls.StackPanel tabBar;
        private Windows.UI.Xaml.Controls.Grid tabContent;
        AppKeyBindings bindings = new AppKeyBindings();
        private int focusedTab = 0;

        public TerminalApp()
        {
            this.tabs = new List<Tab>();

            bindings = new AppKeyBindings();
            bindings.SetKeyBinding(ShortcutAction.NewTab, new KeyChord(KeyModifiers.Ctrl, (int)'T'));
            bindings.SetKeyBinding(ShortcutAction.CloseTab, new KeyChord(KeyModifiers.Ctrl, (int)'W'));
            bindings.NewTab += _DoNewTab;
            bindings.CloseTab += _DoCloseTab;

        }

        public void Attach(Windows.UI.Xaml.Controls.Grid grid)
        {
            if (grid != null)
            {
                root = grid;

                tabBar = new Windows.UI.Xaml.Controls.StackPanel();
                tabContent = new Windows.UI.Xaml.Controls.Grid();

                var tabBarRowDef = new Windows.UI.Xaml.Controls.RowDefinition();
                tabBarRowDef.Height = Windows.UI.Xaml.GridLength.Auto;
                root.RowDefinitions.Add(tabBarRowDef);
                root.RowDefinitions.Add(new Windows.UI.Xaml.Controls.RowDefinition());

                root.Children.Add(tabBar);
                root.Children.Add(tabContent);
                Windows.UI.Xaml.Controls.Grid.SetRow(tabBar, 0);
                Windows.UI.Xaml.Controls.Grid.SetRow(tabContent, 1);
                tabBar.Height = 0;

                tabBar.Orientation = Windows.UI.Xaml.Controls.Orientation.Horizontal;
                tabBar.HorizontalAlignment = Windows.UI.Xaml.HorizontalAlignment.Stretch;

                tabContent.VerticalAlignment = Windows.UI.Xaml.VerticalAlignment.Stretch;
                tabContent.HorizontalAlignment = Windows.UI.Xaml.HorizontalAlignment.Stretch;

                //tabBar.Background = new Windows.UI.Xaml.Media.SolidColorBrush(Windows.UI.Colors.Blue);
                //tabContent.Background = new Windows.UI.Xaml.Media.SolidColorBrush(Windows.UI.Colors.Red);

                _DoNewTab(this);
                
            }
        }

        private void _CreateTabBar()
        {
            tabBar.Children.Clear();
            tabBar.Height = (tabs.Count > 1)? 26 : 0; // 32 works great for the default button text size

            for (int i = 0; i < tabs.Count(); i++)
            {
                Tab tab = tabs[i];
                tabBar.Children.Add(tab.tabButton);
            }
        }

        private bool _DoNewTab(object sender)
        {
            TerminalSettings settings = new TerminalSettings();
            settings.KeyBindings = bindings;

            if (tabs.Count() < 1)
            {
                //// ARGB is 0xAABBGGRR, don't forget
                settings.DefaultBackground = 0xff008a;
                settings.UseAcrylic = true;
                settings.TintOpacity = 0.5;
                //settings.FontSize = 14;
                //settings.FontFace = "Ubuntu Mono";
                // For the record, this works, but ABSOLUTELY DO NOT use a font that isn't installed.
            }
            else
            {
                Random rand = new Random();
                uint bg = (uint)rand.Next(0xffffff);
                bool acrylic = rand.NextDouble() > 0.5;

                settings.DefaultBackground = bg;
                settings.UseAcrylic = acrylic;
                settings.TintOpacity = 0.5;
                //settings.FontSize = 14;
            }

            TermControl term = new TermControl(settings);
            Tab newTab = new Tab(term);
            newTab.tabButton.Click += (object s, Windows.UI.Xaml.RoutedEventArgs e) =>
            {
                _FocusTab(newTab);
            };

            // IMPORTANT: List.Add, Grid.Append.
            // If you reverse these, they silently fail
            tabs.Add(newTab);
            tabContent.Children.Add(term.GetControl());


            if (tabs.Count() > 1)
            {
                _CreateTabBar();
            }

            _FocusTab(newTab);

            return true;
        }

        private void _resetTabs()
        {
            foreach (Tab t in tabs)
            {
                t.tabButton.Background = null;
                t.tabButton.Foreground = new Windows.UI.Xaml.Media.SolidColorBrush(Windows.UI.Colors.White);

                t.tabButton.BorderBrush = null;
                t.tabButton.BorderThickness = new Windows.UI.Xaml.Thickness();

                t.focused = false;
            }
        }

        // TODO @someone who knows c#: does this really just magically make it async? 
        //  what about the call above in _DoNewTab, where it first focuses it.
        private async void _FocusTab(Tab tab)
        {
            await tabBar.Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
            {
                tabContent.Children.Clear();
                tabContent.Children.Add(tab.terminal.GetControl());

                _resetTabs();
                
                //tab.tabButton.Background = new Windows.UI.Xaml.Media.SolidColorBrush(Windows.UI.Colors.DarkSlateGray);
                tab.tabButton.Background = new Windows.UI.Xaml.Media.SolidColorBrush(Windows.UI.Color.FromArgb(255, 0x4f, 0x4f, 0x4f));
                
                tab.tabButton.BorderBrush = new Windows.UI.Xaml.Media.SolidColorBrush(Windows.UI.Colors.Blue);
                tab.tabButton.BorderThickness = new Windows.UI.Xaml.Thickness(0, 2, 0, 0);

                tab.focused = true;

                tab.terminal.GetControl().Focus(Windows.UI.Xaml.FocusState.Programmatic);
            });
            
        }
        private bool _DoCloseTab(object sender)
        {
            if (tabs.Count > 1)
            {
                Tab focusedTab = null;
                for (int i = 0; i < tabs.Count(); i++)
                {
                    Tab tab = tabs[i];
                    if (tab.focused)
                    {
                        focusedTab = tab;
                        break;
                    }
                }
                if (focusedTab == null) return false; //todo wat

                tabs.Remove(focusedTab);
                focusedTab.terminal.Close();
                _CreateTabBar();
                _FocusTab(tabs[0]);

            }

            return true;
        }
    }
}
