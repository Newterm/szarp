<%inherit file="base.mako"/>

<div class="info">
% if c.message:
  <p>${c.message}</p>
% endif
  ${h.form(h.url_for(action= c.action, id = c.id, method='get'))}
  ${h.submit('submit', _('Ok'))}
  ${h.end_form()}
</div>

