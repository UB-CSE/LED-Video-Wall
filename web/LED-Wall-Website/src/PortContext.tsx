import React, { createContext, useContext, useState, ReactNode } from "react";

export type PortsContextType = {
  ledvwPort: number;
  setLedvwPort: (n: number) => void;
  rtmpPort: number;
  setRtmpPort: (n: number) => void;
  isValidPort: (n: number) => boolean;
};

export const PortsContext = createContext<PortsContextType | undefined>(undefined);

type PortsProviderProps = {
  children: ReactNode;
  initialLedvwPort?: number;
  initialRtmpPort?: number;
};

export function PortsProvider({
  children,
  initialLedvwPort = 7070,
  initialRtmpPort = 1935,
}: PortsProviderProps) {
  const [ledvwPort, setLedvwPort] = useState<number>(initialLedvwPort);
  const [rtmpPort, setRtmpPort] = useState<number>(initialRtmpPort);

  const isValidPort = (port: number) =>
    Number.isInteger(port) && port >= 1024 && port <= 65535;

  const value: PortsContextType = {
    ledvwPort,
    setLedvwPort,
    rtmpPort,
    setRtmpPort,
    isValidPort,
  };

  return <PortsContext.Provider value={value}>{children}</PortsContext.Provider>;
}

export function usePorts(): PortsContextType {
  const ctx = useContext(PortsContext);
  if (!ctx) {
    throw new Error("usePorts must be used within a PortsProvider");
  }
  return ctx;
}

export default PortsContext;
