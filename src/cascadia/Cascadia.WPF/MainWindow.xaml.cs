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
        TerminalApp app;

        public MainWindow()
        {
            InitializeComponent();
        }

        private void WindowsXamlHost_XamlRootChanged0(object sender, EventArgs args)
        {
            app = new TerminalApp();
            Windows.UI.Xaml.Controls.Grid grid = (Windows.UI.Xaml.Controls.Grid)terminalHost.Child;
            if (grid != null)
            {
                app.Attach(grid);
            }
        }
    }
}
