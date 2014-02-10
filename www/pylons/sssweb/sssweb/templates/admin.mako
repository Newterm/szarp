<%inherit file="/base.mako"/>

<div class="links">
<%
    ctrl = request.environ['pylons.routes_dict']['controller']
    index = request.environ['pylons.routes_dict']['action'] == 'index'
%>
    <span>${h.link_to_unless(ctrl == 'syncuser' and index, 'Users', h.url(controller='syncuser'))}</span>
    <span>${h.link_to_unless(ctrl == 'syncserver' and index, 'Servers', h.url(controller='syncserver'))}</span>
    <span>${h.link_to_unless(ctrl == 'remarks' and index, 'Remarks', h.url(controller='remarks'))}</span>
    <span>${h.link_to('Logout', h.url(controller='login', action='logout'))}</span>
</div>
${next.body()}

