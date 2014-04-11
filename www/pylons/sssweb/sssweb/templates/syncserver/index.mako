<%inherit file="/admin.mako"/>

<p>${h.link_to('Add server', h.url.current(action = 'new'))}</p>
<table>
  <thead><th>Name</th><th>IP Address</th><th>Bases</th></thead>
% for s in c.list:
    <tr>
%     if s['main']:
      <td><b>${h.link_to(s['name'], h.url.current(action='edit', id = s['name']))}</b></td>
%     else:
      <td>${h.link_to(s['name'], h.url.current(action='edit', id = s['name']))}</td>
%     endif
      <td>${s['ip']}</td>
      <td>${', '.join(s['bases'])}</td>
    </tr>
% endfor
</table>


