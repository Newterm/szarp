<%inherit file="base.mako"/>

<div class="info">
<p>${c.message}</p>
${h.form(h.url.current(action= c.action, id = c.id, method='post'))}
${h.submit('submit', _('Yes'))}
${h.end_form()}
  
${h.form(h.url.current(action= c.no_action, id = c.id, method='get'))}
${h.submit('no_submit', _('No'))}
${h.end_form()}
</div>

