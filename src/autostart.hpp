#pragma once

namespace autostart {

// Devuelve true si el auto-inicio está activado para el ejecutable actual.
bool isEnabled();

// Activa o desactiva el auto-inicio. Devuelve true si la operación tuvo éxito.
bool setEnabled(bool enable);

} // namespace autostart