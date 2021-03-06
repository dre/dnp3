﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

using DNPDotNet;
using DNP3.Interface;
using TestSetControlLibrary;

namespace DotNetTestSet
{
    public partial class TestSetForm : Form
    {
        private StackManager sm;
        private LogControlAdapter lca;

        public TestSetForm()
        {           
            InitializeComponent();

            this.sm = new StackManager();
            this.lca = new LogControlAdapter(this.logControl);
            sm.AddLogHandler(lca);            
        }        

        private void stackBrowser1_OnTcpClientAdded(TcpSettings s)
        {
            sm.AddTCPClient(s.name, s.level, s.timeout, s.address, s.port);                            
        }

        private void stackBrowser1_OnTcpServerAdded(TcpSettings s)
        {
            sm.AddTCPServer(s.name, s.level, s.timeout, s.address, s.port);
        }

        private void stackBrowser1_OnSerialPortAdded(SerialSettings settings)
        {
            sm.AddSerial(settings.port, FilterLevel.LEV_WARNING, 5000, settings);
        }

        private void stackBrowser1_OnAddMaster(string name, string port, FilterLevel level, MasterStackConfig config)
        {
            //create the new form
            var observer = new EventedDataObserver(this);
            var display = new MasterDataDisplay(observer.MeasurementSource); //synchronize all updates on this form


            display.Dock = DockStyle.Fill;
            TabPage page = new TabPage(name);
            this.tabControlDisplay.TabPages.Add(page);
            page.Controls.Add(display);            
            display.CommandAcceptor = sm.AddMaster(port, name, level, observer, config);                            
        }

        private void stackBrowser1_OnRemovePort(string name)
        {
            sm.RemovePort(name);
        }

        private void stackBrowser1_OnRemoveStack(string name)
        {
            sm.RemoveStack(name);
        }

        private void addNewChartToolStripMenuItem_Click(object sender, EventArgs e)
        {
            var chart = new TimeSeriesChartForm();
            chart.Show();
        }

        

        
    }

    
    
}