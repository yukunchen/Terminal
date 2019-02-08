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
    internal class Tab
    {
        public TermControl terminal;
        public Tab(TermControl terminal)
        {
            this.terminal = terminal;
        }
    }

    public class TerminalApp
    {
        private List<Tab> tabs;
        private Windows.UI.Xaml.Controls.Grid root;
        private Windows.UI.Xaml.Controls.StackPanel tabBar;
        private Windows.UI.Xaml.Controls.Grid tabContent;
        AppKeyBindings bindings = new AppKeyBindings();

        public TerminalApp()
        {
            this.tabs = new List<Tab>();

            bindings = new AppKeyBindings();
            bindings.SetKeyBinding(ShortcutAction.NewTab, new KeyChord(KeyModifiers.Ctrl, (int)'T'));
            bindings.NewTab += _DoNewTab;

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
                //tabContent.Height = 320;
                //tabBar.Width = 480;
                //tabContent.Width = 320;
                tabBar.Orientation = Windows.UI.Xaml.Controls.Orientation.Horizontal;
                tabBar.HorizontalAlignment = Windows.UI.Xaml.HorizontalAlignment.Stretch;

                tabContent.VerticalAlignment = Windows.UI.Xaml.VerticalAlignment.Stretch;
                tabContent.HorizontalAlignment = Windows.UI.Xaml.HorizontalAlignment.Stretch;

                tabBar.Background = new Windows.UI.Xaml.Media.SolidColorBrush(Windows.UI.Colors.Blue);
                tabContent.Background = new Windows.UI.Xaml.Media.SolidColorBrush(Windows.UI.Colors.Red);

                _DoNewTab(this);

                //// ARGB is 0xAABBGGRR, don't forget
                //TerminalSettings settings = new TerminalSettings();
                //settings.DefaultBackground = 0xff008a;
                //settings.UseAcrylic = true;
                //settings.TintOpacity = 0.5;
                //settings.FontSize = 14;
                //settings.FontFace = "Ubuntu Mono";
                //// DON'T SET THIS TO A FONT THAT ISN'T INSTALLED
                //// settings.FontFace = "Foo";

                //settings.KeyBindings = bindings;

                //TermControl term = new TermControl(settings);
                //grid.Children.Add(term.GetControl());
                //terminals.Append(term);
            }
        }

        private void _CreateTabBar()
        {
            tabBar.Children.Clear();
            tabBar.Height = 32;
            
            for (int i = 0; i < tabs.Count(); i++)
            {
                Tab t = tabs[i];
                Windows.UI.Xaml.Controls.Button button = new Windows.UI.Xaml.Controls.Button();
                button.Content = $"{i}";
                //button.HorizontalAlignment = Windows.UI.Xaml.HorizontalAlignment.Left;

                tabBar.Children.Add(button);
            }
        }

        private bool _DoNewTab(object sender)
        {
            TerminalSettings settings = new TerminalSettings();
            settings.KeyBindings = bindings;

            if (tabs.Count() < 1)
            {
                settings.DefaultBackground = 0xff008a;
                settings.UseAcrylic = true;
                settings.TintOpacity = 0.5;
                settings.FontSize = 14;
                settings.FontFace = "Ubuntu Mono";
            }
            else
            {
                Random rand = new Random();
                uint bg = (uint)rand.Next(0xffffff);
                bool acrylic = rand.NextDouble() > 0.5;

                settings.DefaultBackground = bg;
                settings.UseAcrylic = acrylic;
                settings.TintOpacity = 0.5;
                settings.FontSize = 14;
            }

            TermControl term = new TermControl(settings);
            Tab newTab = new Tab(term);
            tabs.Add(newTab);
            //tabContent.Children.Clear();
            //var control = newTab.terminal.GetControl();
            //tabContent.Children.Append(control);

            //root.Children.Add(term.GetControl());
            tabContent.Children.Add(term.GetControl());

            if (tabs.Count() > 1)
            {
                _CreateTabBar();
            }
            return true;
        }
    }
}
