<%inherit file="base.mako"/>

<div class="links">
<%
    action = request.environ['pylons.routes_dict']['action']
%>
    <span>${h.link_to_unless(action == 'index', 'Users', h.url_for(action='index'))}</span>
    <span>${h.link_to_unless(action == 'servers', 'Servers', h.url_for(action='servers'))}</span>
    <span>${h.link_to_unless(action == 'remarks', 'Remarks', h.url_for(controller='remarks', action='index'))}</span>
    <span>${h.link_to('Logout', h.url_for(controller='login', action='logout'))}</span>
</div>
${next.body()}

