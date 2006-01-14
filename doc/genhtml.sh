#! /bin/sh

rm -f faq.html

DATE=$( grep "^\$Id" faq.txt | awk '{ print $4 " " $5; }' )

cat > faq.html << EOF
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<head>
<title>HandBrake FAQ</title>
<style type="text/css">
body,p,th,td  { font-family: Verdana,Arial,Helvetica,sans-serif;
                font-size: 8pt;
                font-weight: normal;
                color: #000000; }
body          { margin-left: 10;
                margin-top: 10;
                margin-right: 10;
                margin-bottom: 10;
                background-color: #FFFFFF; }
a             { color: #000000;
                text-decoration: underline; }
a:hover       { color: #888888; }
</style>
</head>
<body bgcolor="#ffffff">

<p>
Last updated: ${DATE}<br>
The latest version of this FAQ can be found <a
href="http://handbrake.m0k.org/faq.php">here</a>.
</p>
EOF

cat faq.txt | grep -v "^\$Id" >> faq.html

cat >> faq.html << EOF
</body>
</html>
EOF

