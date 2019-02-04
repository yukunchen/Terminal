using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Windows.UI.Xaml.Controls;

namespace TerminalWrapper
{
    public sealed class Class1
    {
        public UserControl MakeContent()
        {
            UserControl control = new UserControl();
            Grid grid = new Grid();
            Button button = new Button();
            button.Content = "My Wrapper Button";
            grid.Children.Add(button);
            control.Content = grid;
            return control;
        }
    }
}
