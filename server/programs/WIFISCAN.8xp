PROGRAM:WIFISCAN
:ClrHome
:Disp "WiFi Scanner"
:Disp "v1.0"
:Disp ""
:Disp "Scanning..."
:Send(15,C)
:GetCalc(Str0)
:ClrHome
:Disp "Available Networks:"
:Disp ""
:Disp Str0
:Disp ""
:Disp "Select network:"
:Input "Index:",N
:If N<1 or N>9
:Then
:Disp "Invalid index"
:Pause
:Goto END
:End
:Disp "Selected:"
:Disp Str0
:Pause
:ClrHome
:Disp "WiFi Scanner"
:Disp "Complete"
:Disp ""
:Disp "Next: Run WIFIPASS"
:Disp "to connect"
:Stop
:Lbl END
:ClrHome
:Disp "Error"
:Disp "Invalid selection"
:Stop
