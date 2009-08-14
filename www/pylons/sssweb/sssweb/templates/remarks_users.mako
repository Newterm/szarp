<%inherit file="s_base.mako"/>

<div class="links">
    <span>${h.link_to('Synchronizer', h.url_for(controller='ssconf', action='index'))}</span>
    <span>${h.link_to('Logout', h.url_for(controller='login', action='logout'))}</span>
</div>

<p>${h.link_to('Add user', h.url_for(controller='remarks', action = 'add_user'))}</p>

<table>
  <thead><th>Users:</th></thead>
% for p in c.users_q:
      <tr><td><b>${h.link_to(p.real_name, url = h.url_for(action = 'user', id = p.id))}</b></td></tr> 
% endfor
</table>
