<%inherit file="base.mako"/>

<%
if c.server['name'] == '':
  add_new = True
else:
  add_new = False
%>

${h.form(h.url_for(action='save_server', id = c.server['name'], method='post'))}
<p>Name ${h.text('name', value = c.server['name'], disabled = not add_new)}</p>
<p>IP address ${h.text('ip', value = c.server['ip'])}</p>
% if c.server['main']:
<p>Primary server</p>
% else:
<p>Secondary server</p> 
% endif
% if not add_new:
<p>Bases: ${', '.join(c.server['bases'])}</p>
% endif
${h.submit('submit', 'Save changes')}
${h.end_form()}
% if not add_new:
  ${h.form(h.url_for(action='remove_server', id=c.server['name'], method='post'))}
  ${h.submit('remove', 'Remove server')}
  ${h.end_form()}
% endif


