<%inherit file="admin.mako"/>

<p>${h.link_to('Add server', h.url_for(action = 'add_server'))}</p>
<table>
  <thead><th>Name</th><th>IP Address</th><th>Bases</th></thead>
% for s in c.list:
    <tr>
%     if s['main']:
      <td><b>${h.link_to(s['name'], h.url_for(action='server', id = s['name']))}</b></td>
%     else:
      <td>${h.link_to(s['name'], h.url_for(action='server', id = s['name']))}</td>
%     endif
      <td>${s['ip']}</td>
      <td>${', '.join(s['bases'])}</td>
    </tr>
% endfor
</table>


