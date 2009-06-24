<html>
<head>
<title>${_('SZARP Synchroniser')}</title>
${h.stylesheet_link(h.url_for("/css/default.css"))}
</head>
<body>
<div class="header">${_('SZARP Synchroniser Web Admin')}</div>
% if c.error:
    <div class="error">${c.error}</div>
% endif
  ${next.body()}
</body>
</html>

