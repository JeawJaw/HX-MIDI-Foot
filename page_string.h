const char update_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">
<head>
    <title>Pigman MIDI Footswitch App</title>
    <meta charset="UTF=8">
    <meta name="viewport" content="width=device-width, initial-scale=1">

    <script src="src/jquery-3.7.1.min.js"></script>
    <script src="src/bootstrap.bundle.min.js"></script>
    <link rel="stylesheet" type="text/css" href="src/bootstrap.min.css">

    <link href="css/fontawesome.css" rel="stylesheet" />

    <style type="text/css">
        html 
        { 
          font-family: Helvetica; 
          /* display: inline-block; 
          margin: 0px auto;  */
          
          text-align: center;
          align-content: center;
          align-items: center;
          background-color: #2c3e50;
        }
         body
        {
          margin-top: 10px;
        }
        #bar,#prgbar
        {
            background-color:#f1f1f1;
            border-radius:15px
        }
        #bar
        {
            background-color:#3498db;
            width:0%;
            height:15px
        }
        #file-input,input
        {
            width:100%;
            height:44px;
            border-radius:4px;
            margin:10px auto;
            font-size:15px
        }
        #file-input
        {
            padding:0;
            border:1px solid #ddd;
            line-height:44px;
            text-align:left;
            display:block;
            cursor:pointer;
            background-color: lightslategrey;
            
        }
        input
        {
            background:#f1f1f1;
            border:0;
            padding:0 15px
        }
        

    </style>

</head>
<body>
    <nav class="navbar navbar-dark bg-dark">
        <a class="navbar-brand" href="main_page.html">
            <div class="row ">
                <div class="col-4">
                    <img src="src/PIGManLogo.png" width="80" height="60" class="d-inline-block align-top" alt="">
                </div>
                <div class="col-8" style="vertical-align: middle; text-align: left;">
                    <h2>PMX Editor Version 1.0</h2>by Pigman Pedal
                </div>
            </div>
        </a>
    </nav>
    <div class=" jumbotron jumbotron-fluid  bg-dark ">
        <div class="container">
            <div class="form-group">
                <div class="card bg-secondary bg-gradient" style="width: 100%; margin-bottom: 20px;">
                    <div class="card-header">
                        <h5 style="color: #ffffff;">Update Firmware</h5>
                    </div>
                    <form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>
                        <div class="card-body">
                            <div class="row">
                                <div class="col-6">
                                    <input type='file' id="file" name='update' accept=".bin" class="form-control"  onchange='sub(this);'  style='display:none'>
                                    <label id='file-input' for='file'>Choose File...</label>
                                </div>
                                <div class="col-3">
                                    <input type='submit' value='Update' class="btn btn-success bg-gradient">                                    
                                </div>
                                <div class="col-3">
                                    <a href="main_page.html" class="btn btn-secondary bg-gradient" 
                                    style="width:100%;height:44px;border-radius:4px;margin:10px auto;font-size:15px">Back</a>
                                </div>  
                            </div>
                            <div class="row">
                                <div id='prg' class="col-4" style="text-align: left;">Update Progress :</div>
                            </div>
                            <div class="row">
                                <div id='prgbar' class="bg-gradient"><div id='bar' class="bg-gradient"></div></div>
                            </div>
                    
                        </div>
                    </form>
                </div>
                <br/>
            </div>
        </div>
    </div>
    <div class="bg-dark bg-gradient" style="color:lightsteelblue">
        <strong>Copyright &copy; 2024 <a href="main_page.html">PMX Editor</a>.</strong>
        All rights reserved. <b>Version</b> 1.0
    </div> 
    
    <script>
        function sub(obj)
        {
            var fileName = obj.value.split('\\');
            //var fileName = obj.value;
            document.getElementById('file-input').innerHTML = '     '+ fileName[fileName.length-1];
        };

        function submit_check(obj)
        {
          var 
        }

        $('form').submit(function(e)
        {
            e.preventDefault();
            var form = $('#upload_form')[0];
            var data = new FormData(form);
            $.ajax(
            {
                url: '/update_submit',
                type: 'POST',
                data: data,
                contentType: false,
                processData:false,
                xhr: function() 
                {
                    var xhr = new window.XMLHttpRequest();
                    xhr.upload.addEventListener('progress', function(evt) 
                    {
                        if (evt.lengthComputable) 
                        {
                            var per = evt.loaded / evt.total;
                            $('#prg').html('Update Progress: ' + Math.round(per*100) + '%');
                            $('#bar').css('width',Math.round(per*100) + '%');
                        }
                    }, false);
                    return xhr;
                },
                success:function(d, s) 
                {
                    console.log('success!') 
                    alert('Update Firmware Success');
                },
                error: function (a, b, c) 
                {
                    alert('Update Firmware Error');
                }
            });
       });
       </script>
</body>
</html>)rawliteral";