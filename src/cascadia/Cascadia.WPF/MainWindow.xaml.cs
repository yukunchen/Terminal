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

                term = new TermControl(settings);
                grid.Children.Add(term.GetControl());

            }
        }
    }
}
