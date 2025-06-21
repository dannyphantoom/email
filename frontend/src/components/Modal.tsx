import React from 'react';

interface ModalProps {
  isOpen: boolean;
  onClose: () => void;
  title?: string;
  children: React.ReactNode;
  width?: string;
}

const Modal: React.FC<ModalProps> = ({ isOpen, onClose, title, children, width = 'max-w-md' }) => {
  if (!isOpen) return null;
  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center bg-black bg-opacity-60">
      <div className={`bg-dark-900 rounded-lg shadow-lg w-full ${width} mx-4 p-6 relative`}>
        <button
          className="absolute top-3 right-3 text-dark-400 hover:text-white text-xl"
          onClick={onClose}
          aria-label="Close"
        >
          &times;
        </button>
        {title && <h2 className="text-lg font-semibold text-white mb-4">{title}</h2>}
        {children}
      </div>
    </div>
  );
};

export default Modal; 