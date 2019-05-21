<!DOCTYPE html >
<html>
<head>
<meta charset=utf-8" />
<title>flash</title>
<script src="test.js"></script>
<script src="jquery-3.3.1.min.js"></script>
    <script>
        var wifiShowing = 0;
        
        Date.prototype.Format = function (fmt) { //author: meizz 
            var o = {
                "M+": this.getMonth() + 1, //月份 
                "d+": this.getDate(), //日 
                "H+": this.getHours(), //小时 
                "m+": this.getMinutes(), //分 
                "s+": this.getSeconds(), //秒 
                "q+": Math.floor((this.getMonth() + 3) / 3), //季度 
                "S": this.getMilliseconds() //毫秒 
            };
            if (/(y+)/.test(fmt)) fmt = fmt.replace(RegExp.$1, (this.getFullYear() + "").substr(4 - RegExp.$1.length));
            for (var k in o)
            if (new RegExp("(" + k + ")").test(fmt)) fmt = fmt.replace(RegExp.$1, (RegExp.$1.length == 1) ? (o[k]) : (("00" + o[k]).substr(("" + o[k]).length)));
            return fmt;
        }

        function fetchHeader(url, wch) {
            try {
                var req=new XMLHttpRequest();
                req.open("HEAD", url, false);
                req.send(null);
                if(req.status== 200){
                    return req.getResponseHeader(wch);
                }
                else return false;
            } catch(er) {
                return er.message;
            }
        }
        function get_info() {
            $.ajax({
                type: "GET",
                url: "tmp/upgradetool/result",
                async : false,
                cache: false,
                timeout:1000,
                success: function(dates) {
                    //$("#timeContent").html(xhr.getResponseHeader("Last-Modified"));
                    $("#mainContent").html(dates);
                },
                error: function() {
                    $("#mainContent").html("读取失败！");
                }
            });
        }
        function get_status() {
            $.ajax({
                type: "GET",
                url: "tmp/upgradetool/cdevice",
                async : false,
                cache: false,
                timeout:1000,
                success: function(dates) {
                    //$("#timeContent").html(xhr.getResponseHeader("Last-Modified"));
                    $("#statusContent").html(dates);
                },
                error: function() {
                    $("#statusContent").html("读取失败！");
                }
            });
        }
        function get_time() {
                var xhr = $.ajax({
                    url: "tmp/upgradetool/result",
                    cache: false,
                    success: function(response) {
                        var gmt = xhr.getResponseHeader("Last-Modified");
                        var date = new Date(gmt);
                        $("#timeContent").html(date.Format("yyyy-MM-dd HH:mm:ss"));
                    }
                });
        }

        function tr_click(n){
            alert(n);
        }
        function get_wifi() {
            $.ajax({
                type: "GET",
                url: "tmp/wifimanager/result",
                async : false,
                cache: false,
                timeout:1000,
                success: function(dates) {
                    var tb = document.getElementById('Tab_Main');
                    var rowNum=tb.rows.length;
                    for (i=0;i<rowNum;i++)
                    {
                        tb.deleteRow(i);
                        rowNum=rowNum-1;
                        i=i-1;
                    } 
                    var Table = document.getElementById("Tab_Main");
                    //Table.firstChild.removeNode(true);
                    NewRow = Table.insertRow();
                    NewCell = NewRow.insertCell();
                    NewCell.innerHTML = "SSID";
                    NewCell = NewRow.insertCell();
                    NewCell.innerHTML = "BSSID";
                    NewCell = NewRow.insertCell();
                    NewCell.innerHTML = "SIGNAL";
                    NewCell = NewRow.insertCell();
                    NewCell.innerHTML = "SECURITY";
                    NewCell = NewRow.insertCell();
                    NewCell.innerHTML = "ACTIVE";
                    //$("#timeContent").html(xhr.getResponseHeader("Last-Modified"));
                    var arr = dates.split("\n");
                    for(j = 0; j < arr.length; j++) {
                        var arr1 = arr[j].split("+++");
                        NewRow = Table.insertRow();
                        NewRow.id = j + 1;
                        for(i=0; i<arr1.length; i++) {
                            NewCell = NewRow.insertCell();
                            NewCell.innerHTML = arr1[i];
                            if (arr1[i] == 'connected') {
                                //NewCell.style.color="red"; 
                                NewRow.style.color="red";
                            }else if(arr1[i] == 'conecting')
                            {
                                NewRow.style.color="green";
                            }
                        }
                        NewRow.onclick = function () {
                            var cells = Table.rows[this.id].cells;
                            var ssid = cells[0].innerHTML;
                            var bssid = cells[1].innerHTML;
                            var signal = cells[2].innerHTML;
                            var security = cells[3].innerHTML;
                            var active = cells[4].innerHTML;
                            if (active == "connected") {
                                //do nothing
                            } 
                            else if (security == "--" || active == "remembered") {
                                //no password need
                                //fun5(this.id);
                                fun6(this.id);
                                //alert(security);
                            }
                            else {
                                var password = prompt("Please input password","");
                                if(password != null)
                                {
                                    fun5(bssid, password);
                                }
                                alert("need password");
                                //need password
                            }
                            //alert(ssid + signal + security + active);
                        } 
                    }
                },
                error: function() {
                }
            });
        }

        function fun(n) {
            $.ajax({
                url:"server.php",           //the page containing php script
                type: "POST",               //request type
                data:{action: n.value},
                success:function(result){
                    alert(result);
                }
            });
        }
        
        function fun2(n) {
            var url = "server.php";
            var data = {
                action : n.value
            };
            jQuery.post(url, data, callback);
        }
        function fun3(n) {
            var url = "server.php";
            var data = {
                action : n.value
            };
            jQuery.post(url, data, callback);
        }
        function fun4(n) {
            var url = "server.php";
            var data = {
                action : n.value
            };
            jQuery.post(url, data, callback);
        }
        function fun5(x, y) {
            var url = "server.php";
            var data = {
                action : "btn5",
                bssid : x,
                passwd: y
            };
            jQuery.post(url, data, callback);
        }
        function fun6(n) {
            var url = "server.php";
            var data = {
                action : "btn6",
                bssid : n
            };
            jQuery.post(url, data, callback);
        }
        function callback(data) {
            alert(data);
            if (data == "ready to execute show wifi now") {
                wifiShowing = 1;
                get_wifi();
                var Table = document.getElementById("Tab_Main");
                Table.style.display= '';
                var t2 = setInterval(wifiRefresh, 500);
            }

            if (data == "ready to execute hide wifi now") {
                wifiShowing = 0;
                var Table = document.getElementById("Tab_Main");
                Table.style.display = 'none';
                clearInterval(t2);
            }
        }

        function mainRefresh() {
            get_info();
	        get_time();
            get_status();
        }

        function wifiRefresh() {
            get_wifi();
        }

        $(document).ready(function() {
            var t1 = setInterval(mainRefresh, 500);
        });
    </script>
</head>
<body> 
    <div style="white-space:pre">Local IP addresses : </div>

    <div style="white-space:pre; font-weight: bold; color: red" id="ipContent">IP addresses will be printed here... </div><p/>

    <div style="border-style: groove" align="center">
        <div style="white-space:pre; font-weight: bold; color: red" id="timeContent"></div><p/>
        <div style="white-space:pre" id="mainContent">xxxxx... </div>
    </div><p/>

    <div style="border-style: groove" align="center">
        <div style="white-space:pre" id="statusContent">xxxxx... </div>
    </div><p/>

    <button type="button" class="btn" id="btn1" onclick="fun(this)"  value="btn1">Upgrade</button>
    <button type="button" class="btn" id="btn2" onclick="fun2(this)" value="btn2">EraseFlash</button><p/>
    <button type="button" class="btn" id="btn3" onclick="fun3(this)" value="btn3">ShowWifi</button>
    <button type="button" class="btn" id="btn4" onclick="fun4(this)" value="btn4">HideWifi</button><p/>

    <table id="Tab_Main" style="display:none" border="1" ></table>

</body>

</html>
