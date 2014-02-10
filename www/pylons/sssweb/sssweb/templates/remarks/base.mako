<html>
<head>
<title>${_('SZARP Remarks configuration')}</title>
${h.stylesheet_link(h.url("/css/default.css"))}
</head>
<body>
<div class="header">${_('SZARP Synchroniser Web Admin')}</div>
% if error is not UNDEFINED:
    <div class="error">${error}</div>
% endif
  ${next.body()}
</body>
</html>

