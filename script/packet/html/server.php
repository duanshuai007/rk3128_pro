<?php 
    if (isset($_POST['action']))
    {
        switch($_POST['action'])
        {
            case "btn1":btn1();break;
            case "btn2":btn2();break;
            case "btn3":btn3();break;
            case "btn4":btn4();break;
            case "btn5":btn5($_POST['bssid'], $_POST['passwd']);break;
            case "btn6":btn6($_POST['bssid']);break;
            default:break;
        }
    }
    
    function btn1()
    {
        $myfile = fopen("tmp/upgradetool/action", "w") or die("cannot execute upgrade now!");
        $txt = "update\n";
        fwrite($myfile, $txt);
        fclose($myfile);
        echo "ready to execute upgrade now";
    }
    function btn2()
    {
        $myfile = fopen("tmp/upgradetool/action", "w") or die("cannot execute erase flash now!");
        $txt = "erase\n";
        fwrite($myfile, $txt);
        fclose($myfile);
        echo "ready to execute erase flash now";
    }
    function btn3()
    {
        $myfile = fopen("tmp/wifimanager/action", "w") or die("cannot execute show wifi now!");
        $txt = "list start\n";
        fwrite($myfile, $txt);
        fclose($myfile);
        echo "ready to execute show wifi now";
    }
    function btn4()
    {
        $myfile = fopen("tmp/wifimanager/action", "w") or die("cannot execute hide wifi now!");
        $txt = "list stop\n";
        fwrite($myfile, $txt);
        fclose($myfile);
        echo "ready to execute hide wifi now";
    }
    function btn5($bssid, $passwd)
    {
        $myfile = fopen("tmp/wifimanager/action", "w") or die("cannot execute input wifi now!");
        $txt = "connect bssid=".$bssid.",password=".$passwd;
        fwrite($myfile, $txt);
        fclose($myfile);
        echo "ready to execute input wifi now";
    }
    function btn6($bssid)
    {
        $myfile = fopen("tmp/wifimanager/action", "w") or die("cannot execute connect wifi now!");
        $txt = "connect bssid=".$bssid;
        fwrite($myfile, $txt);
        fclose($myfile);
        echo "ready to execute connect wifi now";
    }
 
?>
